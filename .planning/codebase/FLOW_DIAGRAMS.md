# OnlyWan 模型调用流程图

**生成日期:** 2026-03-28

## 1. 各个模型调用流程图

### 1.1 模型注册与创建流程

```
┌─────────────────────────────────────────────────────────────┐
│                    Model Registry System                     │
│                  (src/model_registry.hpp)                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Model Factory Registration                      │
│              (src/model_factory.cpp)                         │
│                                                              │
│  REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-*")      │
│  REGISTER_MODEL_FACTORY(T5Runner, "t5-*")                   │
│  REGISTER_MODEL_FACTORY(WanVAERunner, "wan-vae-*")          │
│  REGISTER_MODEL_FACTORY(WanRunner, "wan-runner-*")          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│         ModelRegistry::instance()->create<T>(version)        │
│                                                              │
│  Input: version string (e.g., "clip-vit-l-14")             │
│  Output: unique_ptr<ModelRunner>                            │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 四个模型的调用接口

```
┌──────────────────────────────────────────────────────────────────┐
│                    Model Runner Hierarchy                        │
│                                                                  │
│                      GGMLRunner (Base)                           │
│                            │                                     │
│        ┌───────────────────┼───────────────────┐                │
│        │                   │                   │                │
│        ▼                   ▼                   ▼                │
│   CLIPTextModelRunner  T5Runner         WAN::WanVAERunner      │
│                                                │                │
│                                                ▼                │
│                                        WAN::WanRunner           │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 1.3 CLIP 文本编码器调用流程

```
输入文本提示 (Text Prompt)
    │
    ▼
CLIPTokenizer
    │
    ├─ 分词 (Tokenization)
    │  └─ 使用 BPE (Byte Pair Encoding)
    │
    ▼
Token IDs [batch=1, seq_len=77]
    │
    ▼
CLIPTextModelRunner::forward()
    │
    ├─ 参数: input_ids, embeddings, mask, max_token_idx, return_pooled, clip_skip
    │
    ├─ 处理流程:
    │  ├─ Token Embedding (vocab_size=49408 → hidden_size)
    │  ├─ Position Embedding (max_position_embeddings=77)
    │  ├─ Transformer Encoder Blocks (12-32 layers)
    │  │  ├─ Self-Attention
    │  │  ├─ Feed-Forward Network
    │  │  └─ Layer Normalization
    │  ├─ Final Layer Normalization
    │  └─ 可选: Text Projection (return_pooled=true)
    │
    ▼
文本嵌入 [batch=1, seq_len=77, hidden_size=768/1024/1280]
    │
    └─ 用于: WAN Transformer 的交叉注意力输入
```

**模型变体:**
- `clip-vit-l-14` - ViT-L (hidden_size=768, 12 layers)
- `clip-vit-h-14` - ViT-H (hidden_size=1024, 24 layers)
- `clip-vit-bigg-14` - ViT-BigG (hidden_size=1280, 32 layers)

**注意:** 虽然代码中定义了 `CLIPVisionModel`（图像编码器），但在模型工厂中只注册了 `CLIPTextModelRunner`（文本编码器）。

### 1.4 T5 文本编码器调用流程

```
输入文本
    │
    ▼
T5UniGramTokenizer / SentencePiece
    │
    ├─ 分词 (Tokenization)
    │
    ▼
Token IDs [batch=1, seq_len=16]
    │
    ▼
T5Runner::forward()
    │
    ├─ 参数: input_ids, relative_position_bucket, attention_mask
    │
    ├─ 处理流程:
    │  ├─ Token Embedding
    │  ├─ Relative Position Buckets
    │  ├─ Transformer Encoder Blocks (12-24 layers)
    │  └─ Output Hidden States
    │
    ▼
隐藏状态 [batch=1, seq_len=16, model_dim=4096]
    │
    └─ 用于: WAN Transformer 的交叉注意力输入
```

**模型变体:**
- `t5-standard` - 标准 T5 (model_dim=4096)
- `t5-umt5` - 多语言 UMT5 (model_dim=4096)

### 1.5 WAN VAE 编码/解码流程

```
┌─────────────────────────────────────────────────────────────┐
│              WAN VAE (Video Autoencoder)                    │
│                                                              │
│  模型变体:                                                   │
│  - wan-vae-t2v (完整编码/解码)                              │
│  - wan-vae-t2v-decode (仅解码)                              │
│  - wan-vae-i2v (图像到视频)                                 │
│  - wan-vae-ti2v (文本+图像到视频)                           │
└─────────────────────────────────────────────────────────────┘

编码路径 (可选):
    │
    ▼
输入视频帧 [batch, channels, time, height, width]
    │
    ▼
WanVAERunner::compute(encode=true)
    │
    ├─ Encoder3d:
    │  ├─ Conv3d 下采样
    │  ├─ Residual Blocks
    │  └─ Attention Blocks
    │
    ▼
潜在表示 z [batch, latent_channels=16, t, h, w]
    │
    └─ 用于: WAN Transformer 的输入

解码路径:
    │
    ▼
潜在表示 z [batch, 16, time, height, width]
    │
    ▼
WanVAERunner::compute(decode=true)
    │
    ├─ Decoder3d:
    │  ├─ Conv3d 上采样
    │  ├─ Residual Blocks
    │  ├─ Attention Blocks
    │  └─ 最终投影
    │
    ▼
解码视频帧 [batch, channels, time, height, width]
    │
    └─ 最终输出
```

### 1.6 WAN Transformer (DiT) 调用流程

```
输入:
  ├─ x: 潜在表示 [batch, channels=16, time, height, width]
  ├─ timesteps: 扩散步数 [batch] (int32)
  └─ context: 文本嵌入 [batch, seq_len, text_dim]

    │
    ▼
WAN::WanRunner::compute()
    │
    ├─ 1. Patch Embedding
    │  └─ x → [batch, num_patches, dim]
    │
    ├─ 2. Time Embedding
    │  ├─ timesteps → [batch, dim]
    │  └─ 可选: 缓存位置编码 (PE caching)
    │
    ├─ 3. Text Embedding 投影
    │  └─ context → [batch, seq_len, dim]
    │
    ├─ 4. Positional Encoding (RoPE)
    │  └─ 生成旋转位置编码
    │
    ├─ 5. Transformer Blocks (30-40 layers)
    │  ├─ Self-Attention
    │  ├─ Cross-Attention (with text context)
    │  ├─ FFN (Feed-Forward Network)
    │  └─ 可选: VACE (Video Augmented Cross-attention)
    │
    ├─ 6. Head Projection
    │  └─ → [batch, num_patches, output_channels]
    │
    ▼
Unpatchify:
    │
    └─ → [batch, channels, time, height, width]

输出:
    │
    └─ 预测的噪声/残差 [batch, 16, time, height, width]
```

**模型变体:**
- `wan-runner-t2v` - 文本到视频 (14B, 40 layers)
- `wan-runner-i2v` - 图像到视频 (14B, 40 layers)
- `wan-runner-ti2v` - 文本+图像到视频 (5B, 30 layers)

**模型规格:**
- 1.3B: 30 layers, dim=1536, ffn_dim=8960, heads=12
- 14B: 40 layers, dim=5120, ffn_dim=13824, heads=40
- 5B (TI2V): 30 layers, dim=3072, ffn_dim=14336, heads=24

---

## 2. WAN 全流程调用流程图 (T2V 示例)

### 2.1 完整推理管道

```
┌──────────────────────────────────────────────────────────────────┐
│                    WAN T2V 完整推理流程                          │
└──────────────────────────────────────────────────────────────────┘

第一步: 文本编码
═══════════════════════════════════════════════════════════════════

输入文本提示 (Prompt)
    │
    ▼
选择文本编码器 (CLIP 或 T5)
    │
    ├─ 选项 A: CLIP 编码
    │  │
    │  ├─ CLIPTokenizer.encode(prompt)
    │  │  └─ Token IDs [1, 77]
    │  │
    │  ├─ CLIPTextModelRunner.forward()
    │  │  └─ 文本嵌入 [1, 77, 768]
    │  │
    │  └─ 输出: text_embeddings_clip
    │
    └─ 选项 B: T5 编码
       │
       ├─ T5UniGramTokenizer.encode(prompt)
       │  └─ Token IDs [1, 16]
       │
       ├─ T5Runner.forward()
       │  └─ 隐藏状态 [1, 16, 4096]
       │
       └─ 输出: text_embeddings_t5

    ▼
文本嵌入 [batch=1, seq_len, text_dim]
    │
    └─ 保存用于后续步骤


第二步: 初始化噪声 (Noise Initialization)
═══════════════════════════════════════════════════════════════════

生成随机噪声
    │
    ▼
噪声张量 x [batch=1, channels=16, time=T, height=H, width=W]
    │
    └─ 形状示例: [1, 16, 16, 64, 64]


第三步: 扩散去噪循环 (Diffusion Denoising Loop)
═══════════════════════════════════════════════════════════════════

for timestep in [999, 998, ..., 1, 0]:
    │
    ├─ 1. 准备输入
    │  ├─ x: 当前潜在表示 [1, 16, T, H, W]
    │  ├─ timesteps: [timestep] (int32)
    │  └─ context: 文本嵌入 [1, seq_len, text_dim]
    │
    ├─ 2. 调用 WAN Transformer
    │  │
    │  ├─ WAN::WanRunner::compute(
    │  │    x, timesteps, context,
    │  │    ..., output, ctx
    │  │  )
    │  │
    │  └─ 输出: 预测噪声 [1, 16, T, H, W]
    │
    ├─ 3. 更新潜在表示
    │  │
    │  ├─ 计算去噪步骤
    │  │  └─ x_t-1 = (x_t - sqrt(1-alpha_bar_t) * noise) / sqrt(alpha_bar_t)
    │  │
    │  └─ x = x_t-1
    │
    └─ 进度回调: progress_cb(step, total_steps)

    ▼
最终潜在表示 z [1, 16, T, H, W]


第四步: VAE 解码 (VAE Decoding)
═══════════════════════════════════════════════════════════════════

潜在表示 z [1, 16, T, H, W]
    │
    ▼
WAN::WanVAERunner::compute(
    n_threads=4,
    z,
    decode_only=true,
    output,
    ctx
)
    │
    ├─ Decoder3d 处理:
    │  ├─ Conv3d 上采样 (16 → 8 → 4 → 3 channels)
    │  ├─ Residual Blocks
    │  ├─ Attention Blocks
    │  └─ 最终投影
    │
    ▼
解码视频帧 [1, 3, T, H, W]
    │
    └─ 像素值范围: [0, 255] 或 [-1, 1]


第五步: 输出处理 (Output Processing)
═══════════════════════════════════════════════════════════════════

解码视频帧 [1, 3, T, H, W]
    │
    ├─ 归一化 (如需要)
    │
    ├─ 转换为 uint8 (0-255)
    │
    ├─ 逐帧提取
    │
    ▼
保存为视频文件 (AVI/MP4)
    │
    └─ 最终输出: video.avi
```

### 2.2 数据流详细图

```
┌─────────────────────────────────────────────────────────────────┐
│                      数据流详细图                               │
└─────────────────────────────────────────────────────────────────┘

输入层:
┌──────────────────────────────────────────────────────────────┐
│ 文本提示 (Prompt)                                            │
│ 例: "A cat running in the forest"                           │
└──────────────────────────────────────────────────────────────┘
         │
         ├─────────────────────────────────────────┐
         │                                         │
         ▼                                         ▼
    ┌─────────────┐                         ┌─────────────┐
    │ CLIP 编码器  │                         │ T5 编码器   │
    │ (768 dim)   │                         │ (4096 dim)  │
    └─────────────┘                         └─────────────┘
         │                                         │
         └─────────────────────────────────────────┘
                         │
                         ▼
            ┌──────────────────────────┐
            │ 文本嵌入                  │
            │ [1, seq_len, text_dim]   │
            └──────────────────────────┘
                         │
                         │ (保存)
                         │
         ┌───────────────┴───────────────┐
         │                               │
         ▼                               ▼
    ┌─────────────┐              ┌──────────────────┐
    │ 随机噪声    │              │ WAN Transformer  │
    │ [1,16,T,H,W]│              │ (DiT)            │
    └─────────────┘              └──────────────────┘
         │                               │
         └───────────────┬───────────────┘
                         │
                    (循环 1000 步)
                         │
                         ▼
            ┌──────────────────────────┐
            │ 去噪潜在表示              │
            │ [1, 16, T, H, W]         │
            └──────────────────────────┘
                         │
                         ▼
            ┌──────────────────────────┐
            │ WAN VAE 解码器           │
            │ (Decoder3d)              │
            └──────────────────────────┘
                         │
                         ▼
            ┌──────────────────────────┐
            │ 视频帧                   │
            │ [1, 3, T, H, W]          │
            │ (RGB, 0-255)             │
            └──────────────────────────┘
                         │
                         ▼
            ┌──────────────────────────┐
            │ 输出视频文件             │
            │ video.avi                │
            └──────────────────────────┘
```

### 2.3 关键参数和配置

```
┌──────────────────────────────────────────────────────────────┐
│                    关键参数配置                              │
└──────────────────────────────────────────────────────────────┘

文本编码参数:
├─ CLIP:
│  ├─ max_token_length: 77
│  ├─ hidden_size: 768/1024/1280
│  └─ clip_skip: -1 (使用最后一层)
│
└─ T5:
   ├─ max_token_length: 16
   ├─ model_dim: 4096
   └─ relative_position_buckets: 32

WAN Transformer 参数:
├─ 输入形状: [batch, 16, time, height, width]
├─ 时间步: 0-999 (1000 步扩散)
├─ 模型维度: 1536/3072/5120
├─ 层数: 30/40
├─ 注意力头数: 12/24/40
└─ 位置编码: RoPE (旋转位置编码)

VAE 参数:
├─ 输入: 潜在表示 [batch, 16, T, H, W]
├─ 输出: 视频帧 [batch, 3, T, H, W]
├─ 解码模式: decode_only=true
└─ 线程数: 4

推理配置:
├─ 批大小: 1
├─ 时间步长: 16 帧
├─ 空间分辨率: 64x64 (潜在空间)
├─ 最终分辨率: 512x512 (像素空间)
└─ 总步数: 1000 (扩散步数)
```

### 2.4 性能优化点

```
┌──────────────────────────────────────────────────────────────┐
│                    性能优化策略                              │
└──────────────────────────────────────────────────────────────┘

OP-02: 位置编码缓存 (PE Caching)
├─ 问题: 每个推理步骤重新计算位置编码
├─ 解决: 缓存 RoPE 位置编码
├─ 收益: 减少 CPU 计算和 CPU→GPU 传输
└─ 实现: WAN::WanRunner 中的 pe_cached

FUS-02: 算子融合 (Operator Fusion)
├─ 问题: GELU 激活函数单独执行
├─ 解决: 原地 GELU (Inplace GELU)
├─ 收益: 减少内存访问和内核启动
└─ 实现: FFN 块中的融合

CG-02: CUDA 图自动合并 (CUDA Graph Merging)
├─ 问题: 多个小内核导致开销
├─ 解决: CUDA 图自动合并相邻内核
├─ 收益: 减少内核启动开销
└─ 实现: GGML 后端自动处理

内存优化:
├─ 参数卸载: 模型参数可卸载到 CPU
├─ 激活缓存: 特征图缓存用于部分计算
└─ 张量重用: 最大化张量重用率
```

---

## 3. 模型调用时序图

### 3.1 单步推理时序

```
时间轴:
│
├─ T0: 准备输入
│  ├─ x: 潜在表示
│  ├─ timesteps: 当前步数
│  └─ context: 文本嵌入
│
├─ T1: WAN Transformer 前向传播
│  ├─ Patch Embedding
│  ├─ Time Embedding
│  ├─ Transformer Blocks (30-40 层)
│  │  ├─ Self-Attention
│  │  ├─ Cross-Attention
│  │  └─ FFN
│  └─ Head Projection
│
├─ T2: 获取输出
│  └─ 预测噪声 [batch, 16, T, H, W]
│
└─ T3: 更新潜在表示
   └─ x_t-1 = denoise(x_t, noise_pred)
```

### 3.2 完整推理时序

```
总时间: ~30-60 秒 (取决于硬件)

│
├─ 文本编码 (1-2 秒)
│  ├─ CLIP/T5 前向传播
│  └─ 输出: 文本嵌入
│
├─ 扩散循环 (25-55 秒)
│  ├─ 1000 步迭代
│  │  ├─ 每步: ~25-55 毫秒
│  │  ├─ WAN Transformer 前向传播
│  │  └─ 去噪更新
│  └─ 进度回调每 10 步
│
├─ VAE 解码 (2-5 秒)
│  ├─ Decoder3d 处理
│  └─ 输出: 视频帧
│
└─ 输出处理 (<1 秒)
   └─ 保存为视频文件
```

---

## 4. 错误处理和验证

```
┌──────────────────────────────────────────────────────────────┐
│                    错误处理流程                              │
└──────────────────────────────────────────────────────────────┘

模型加载:
├─ 检查: 模型版本是否注册
├─ 检查: 模型文件是否存在
├─ 检查: 张量存储映射是否完整
└─ 错误: 抛出异常或返回 nullptr

推理验证:
├─ 检查: 输入张量形状
├─ 检查: 输入张量类型
├─ 检查: 批大小 (必须为 1)
└─ 错误: GGML_ASSERT 或异常

输出验证:
├─ 检查: 输出张量形状
├─ 检查: 输出值范围
└─ 错误: 日志记录或异常
```

---

*流程图分析: 2026-03-28*

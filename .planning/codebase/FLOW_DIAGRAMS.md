# OnlyWan 模型调用流程图

**更新日期:** 2026-03-28

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
│  REGISTER_MODEL_FACTORY(CLIPVisionModelProjectionRunner,    │
│                         "clip-vision-*") ✅ 已注册           │
│  REGISTER_MODEL_FACTORY(T5Runner, "t5-*")                   │
│  REGISTER_MODEL_FACTORY(WanVAERunner, "wan-vae-*")          │
│  REGISTER_MODEL_FACTORY(WanRunner, "wan-runner-*")          │
│                                                              │
│  注意: CLIPTextModelRunner 已注册但在 WAN 推理中未使用      │
│       CLIPVisionModelProjectionRunner 用于 I2V/TI2V 的图像编码 │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│         ModelRegistry::instance()->create<T>(version)        │
│                                                              │
│  Input: version string (e.g., "clip-vision-vit-l-14")      │
│  Output: unique_ptr<ModelRunner>                            │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 模型调用接口层级

```
┌──────────────────────────────────────────────────────────────────┐
│                    Model Runner Hierarchy                        │
│                                                                  │
│                      GGMLRunner (Base)                           │
│                            │                                     │
│        ┌───────────────────┼───────────────────┐                │
│        │                   │                   │                │
│        ▼                   ▼                   ▼                │
│   CLIPVisionModelProjectionRunner  T5Runner  WAN::WanVAERunner  │
│   (已注册) ✅ NEW           (必需)           (必需)             │
│                                                │                │
│                                                ▼                │
│                                        WAN::WanRunner           │
│                                        (必需)                   │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 1.3 CLIP 视觉编码器调用流程

```
输入图像 (Image)
    │
    ▼
图像预处理
    │
    ├─ 加载图像 (JPG/PNG via stb_image)
    │
    ├─ 缩放到 224x224
    │
    ├─ 归一化 (ImageNet normalization)
    │  └─ mean=[0.48145466, 0.4578275, 0.40821073]
    │  └─ std=[0.26862954, 0.26130258, 0.27577711]
    │
    ▼
像素值张量 [batch=1, channels=3, height=224, width=224]
    │
    ▼
CLIPVisionModelProjectionRunner::forward()
    │
    ├─ 参数: pixel_values, return_pooled=true, clip_skip=-1
    │
    ├─ 处理流程:
    │  ├─ Patch Embedding
    │  │  └─ Conv2d: 3 channels → hidden_size (patch_size=14)
    │  │  └─ 输出: [batch, num_positions=257, hidden_size]
    │  │
    │  ├─ Pre-LayerNorm
    │  │
    │  ├─ Vision Transformer Encoder Blocks (12-48 layers)
    │  │  ├─ Self-Attention
    │  │  ├─ Feed-Forward Network
    │  │  └─ Layer Normalization
    │  │
    │  ├─ Post-LayerNorm
    │  │
    │  └─ Visual Projection
    │     └─ Linear: hidden_size → projection_dim
    │
    ▼
图像特征 [batch=1, projection_dim]
    │
    └─ 用于: WAN Transformer 的交叉注意力输入 (clip_fea)
```

**已注册的 CLIP 视觉编码器版本:**
- `clip-vision-vit-l-14` - ViT-L (hidden_size=1024, projection_dim=768, 24 layers)
- `clip-vision-vit-h-14` - ViT-H (hidden_size=1280, projection_dim=1024, 32 layers)
- `clip-vision-vit-bigg-14` - ViT-BigG (hidden_size=1664, projection_dim=1280, 48 layers)

**输出规格:**
- 形状: [batch, projection_dim]
- 用途: I2V 和 TI2V 模式中的图像条件编码
- 集成: 通过 WAN::WanRunner 的 clip_fea 参数传入

### 1.4 T5 文本编码器调用流程

```
输入文本提示 (Text Prompt)
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
  ├─ context: 文本嵌入 [batch, seq_len, text_dim]
  └─ clip_fea: 图像特征 [batch, projection_dim] (仅 I2V/TI2V)

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
    ├─ 4. Image Feature 投影 (仅 I2V/TI2V)
    │  └─ clip_fea → [batch, dim]
    │
    ├─ 5. Positional Encoding (RoPE)
    │  └─ 生成旋转位置编码
    │
    ├─ 6. Transformer Blocks (30-40 layers)
    │  ├─ Self-Attention
    │  ├─ Cross-Attention (with text context)
    │  ├─ Cross-Attention (with image features, 仅 I2V/TI2V)
    │  ├─ FFN (Feed-Forward Network)
    │  └─ 可选: VACE (Video Augmented Cross-attention)
    │
    ├─ 7. Head Projection
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

## 2. 模型使用场景总结

```
┌──────────────────────────────────────────────────────────────┐
│              各模型在不同场景中的使用                        │
│                                                              │
│  重要: CLIP 文本编码器在 WAN 推理中 NOT USED                │
│       T5 是唯一的文本编码器                                 │
│       CLIP 视觉编码器已注册，用于 I2V/TI2V 的图像编码      │
└──────────────────────────────────────────────────────────────┘

场景 1: T2V (Text-to-Video) - 文本生成视频
═══════════════════════════════════════════════════════════════
输入: 文本提示
  │
  ├─ T5Runner (文本编码) ✅ 必需
  │  └─ 输出: 文本嵌入 context [4096, n_token]
  │
  ├─ WAN::WanRunner (T2V 模式)
  │  ├─ 输入: 噪声 + 文本嵌入
  │  └─ 输出: 潜在表示 [1, 16, T, H, W]
  │
  └─ WAN::WanVAERunner (解码)
     └─ 输出: 视频帧 [1, 3, T, H, W]

场景 2: I2V (Image-to-Video) - 图像生成视频
═══════════════════════════════════════════════════════════════
输入: 图像 + 可选文本提示
  │
  ├─ CLIPVisionModelProjectionRunner (图像编码) ✅ 必需 (已注册)
  │  └─ 输出: 图像特征 clip_fea [N, projection_dim]
  │
  ├─ T5Runner (文本编码) ⚠️ 可选
  │  └─ 输出: 文本嵌入 context [4096, n_token]
  │
  ├─ WAN::WanRunner (I2V 模式)
  │  ├─ 输入: 初始帧 + 文本嵌入 + 图像特征
  │  └─ 输出: 潜在表示 [1, 16, T, H, W]
  │
  └─ WAN::WanVAERunner (解码)
     └─ 输出: 视频帧 [1, 3, T, H, W]

场景 3: TI2V (Text+Image-to-Video) - 文本+图像生成视频
═══════════════════════════════════════════════════════════════
输入: 文本提示 + 图像
  │
  ├─ CLIPVisionModelProjectionRunner (图像编码) ✅ 必需 (已注册)
  │  └─ 输出: 图像特征 clip_fea [N, projection_dim]
  │
  ├─ T5Runner (文本编码) ✅ 必需
  │  └─ 输出: 文本嵌入 context [4096, n_token]
  │
  ├─ WAN::WanRunner (TI2V 模式, 5B 模型)
  │  ├─ 输入: 初始帧 + 文本嵌入 + 图像特征
  │  └─ 输出: 潜在表示 [1, 16, T, H, W]
  │
  └─ WAN::WanVAERunner (解码)
     └─ 输出: 视频帧 [1, 3, T, H, W]

关键发现:
✅ T5Runner: 所有场景都需要 (T2V/I2V/TI2V)
❌ CLIPTextModelRunner: 已注册但在 WAN 推理中未使用
✅ CLIPVisionModelProjectionRunner: 仅 I2V/TI2V 需要 (已注册) ✨ NEW
✅ WAN::WanRunner: 所有场景都需要
✅ WAN::WanVAERunner: 所有场景都需要
```

---

## 3. WAN 全流程调用流程图 (T2V 示例)

### 3.1 完整推理管道

```
┌──────────────────────────────────────────────────────────────────┐
│                    WAN T2V 完整推理流程                          │
└──────────────────────────────────────────────────────────────────┘

第一步: 文本编码 (T5 编码器)
═══════════════════════════════════════════════════════════════

输入文本提示 (Prompt)
    │
    ▼
T5UniGramTokenizer / SentencePiece
    │
    ├─ 分词 (Tokenization)
    │  └─ Token IDs [1, 16]
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
文本嵌入 context [4096, n_token]
    │
    └─ 保存用于后续步骤

注意: T5 是唯一的文本编码器，CLIPTextModelRunner 在 WAN 推理中 NOT USED


第二步: 初始化噪声 (Noise Initialization)
═══════════════════════════════════════════════════════════════

生成随机噪声
    │
    ▼
噪声张量 x [batch=1, channels=16, time=T, height=H, width=W]
    │
    └─ 形状示例: [1, 16, 16, 64, 64]


第三步: 扩散去噪循环 (Diffusion Denoising Loop)
═══════════════════════════════════════════════════════════════

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
═══════════════════════════════════════════════════════════════

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
═══════════════════════════════════════════════════════════════

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

### 3.2 数据流详细图

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
         ▼
    ┌─────────────┐
    │ T5 编码器   │
    │ (4096 dim)  │
    └─────────────┘
         │
         └─ 文本嵌入 [1, seq_len, 4096]
                │
                │ (保存)
                │
         ┌──────┴──────┐
         │             │
         ▼             ▼
    ┌─────────────┐  ┌──────────────────┐
    │ 随机噪声    │  │ WAN Transformer  │
    │ [1,16,T,H,W]│  │ (DiT)            │
    └─────────────┘  └──────────────────┘
         │                   │
         └───────────┬───────┘
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

### 3.3 关键参数和配置

```
┌──────────────────────────────────────────────────────────────┐
│                    关键参数配置                              │
└──────────────────────────────────────────────────────────────┘

文本编码参数:
├─ T5:
│  ├─ max_token_length: 16
│  ├─ model_dim: 4096
│  └─ relative_position_buckets: 32

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

### 3.4 性能优化点

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

## 4. I2V/TI2V 推理流程 (使用 CLIP 视觉编码器)

### 4.1 I2V 完整流程

```
输入: 图像 + 可选文本提示
    │
    ├─ 图像预处理
    │  ├─ 加载图像 (JPG/PNG)
    │  ├─ 缩放到 224x224
    │  └─ 归一化
    │
    ├─ CLIP 视觉编码
    │  │
    │  ├─ CLIPVisionModelProjectionRunner::forward()
    │  │  ├─ Patch Embedding
    │  │  ├─ Vision Transformer Blocks
    │  │  └─ Visual Projection
    │  │
    │  └─ 输出: clip_fea [batch, projection_dim]
    │
    ├─ 可选: T5 文本编码
    │  │
    │  ├─ T5Runner::forward()
    │  │  └─ 输出: context [4096, n_token]
    │  │
    │  └─ 如果无文本，使用零向量
    │
    ├─ WAN Transformer (I2V 模式)
    │  │
    │  ├─ 输入: 噪声 + 文本嵌入 + 图像特征
    │  ├─ 扩散循环 (1000 步)
    │  │  ├─ 每步调用 WAN::WanRunner::compute()
    │  │  ├─ 传入 clip_fea 参数
    │  │  └─ 更新潜在表示
    │  │
    │  └─ 输出: 去噪潜在表示
    │
    ├─ VAE 解码
    │  │
    │  ├─ WanVAERunner::compute(decode=true)
    │  │  └─ 输出: 视频帧
    │  │
    │  └─ 最终输出: 视频文件
    │
    └─ 完成
```

---

*流程图最后更新: 2026-03-28*

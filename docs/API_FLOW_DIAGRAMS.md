# OnlyWan API 调用流程图

## 1. 系统整体架构流程图

```mermaid
graph TB
    subgraph Application["应用层"]
        App["用户应用代码"]
    end

    subgraph Registry["模型注册与工厂层"]
        MR["ModelRegistry<br/>单例"]
        MF["ModelFactory<br/>工厂函数"]
    end

    subgraph Runners["模型运行器层"]
        WR["WanRunner<br/>Transformer"]
        VR["WanVAERunner<br/>VAE编解码"]
        CR["CLIPRunner<br/>文本/图像编码"]
        TR["T5Runner<br/>文本编码"]
    end

    subgraph Blocks["组件/块层"]
        AB["WanAttentionBlock<br/>注意力块"]
        CB["Conv/Linear<br/>卷积/线性层"]
        NB["Norm/Activation<br/>归一化/激活"]
    end

    subgraph GGML["GGML扩展层"]
        GE["ggml_extend.hpp<br/>GGML操作包装"]
    end

    subgraph Backend["底层计算库"]
        CPU["CPU"]
        GPU["GPU<br/>CUDA/Metal"]
    end

    App -->|create<T>| MR
    MR -->|查找工厂| MF
    MF -->|创建实例| WR
    MF -->|创建实例| VR
    MF -->|创建实例| CR
    MF -->|创建实例| TR

    WR -->|包含| AB
    WR -->|包含| CB
    VR -->|包含| CB
    CR -->|包含| CB
    TR -->|包含| CB

    AB -->|调用| GE
    CB -->|调用| GE
    NB -->|调用| GE

    GE -->|执行| CPU
    GE -->|执行| GPU
```

## 2. 模型加载流程图

```mermaid
sequenceDiagram
    participant App as 应用代码
    participant MR as ModelRegistry
    participant MF as ModelFactory
    participant Runner as ModelRunner
    participant Block as GGMLBlock
    participant GGML as ggml_context

    App->>MR: create<WanRunner><br/>("wan-runner-t2v", ...)
    MR->>MR: 查找注册的工厂函数
    MR->>MF: 调用工厂 lambda
    MF->>Runner: std::make_unique<WanRunner>(...)
    Runner->>Runner: init()
    Runner->>Block: init_params(ctx, prefix)
    Block->>GGML: 为每个参数分配张量
    Block-->>Runner: 参数张量映射
    Runner-->>MF: 初始化完成
    MF-->>MR: 返回 unique_ptr
    MR-->>App: 返回模型实例

    App->>Runner: alloc_params_buffer()
    Runner->>GGML: 分配缓冲区
    GGML-->>Runner: 缓冲区指针

    App->>Runner: get_param_tensors()
    Runner-->>App: 参数张量映射

    App->>App: ModelLoader::load(weights)
```

## 3. T2V (文本到视频) 推理流程图

```mermaid
graph TD
    subgraph Input["输入准备"]
        Text["文本提示<br/>a cat running"]
        Noise["噪声初始化<br/>x ~ N(0,1)"]
        Params["生成参数<br/>frames, height, width"]
    end

    subgraph TextEnc["文本编码阶段"]
        Tokenize["Tokenize<br/>→ [1, 77]"]
        Embed["Embedding<br/>→ [1, 77, 768]"]
        TextModel["CLIPTextModel<br/>或 T5Model"]
        TextOut["文本嵌入<br/>[N, 512, 4096]"]
    end

    subgraph TransformerDiT["Transformer DiT 推理"]
        PatchEmb["Patch Embedding<br/>[N*16, T, H, W]<br/>→ [N, t_len*h_len*w_len, dim]"]
        TimeEmb["Time Embedding<br/>timestep → [N, dim]"]
        PosEnc["位置编码 RoPE<br/>[pos_len, axes_dim_sum/2, 2, 2]"]
        Blocks["30-40 层 Transformer Blocks<br/>WanAttentionBlock"]
        Head["Head 输出<br/>[N, t_len*h_len*w_len, 16*1*2*2]"]
        Unpatch["Unpatchify<br/>→ [N*16, T, H, W]"]
    end

    subgraph VAEDec["VAE 解码阶段"]
        Conv2["Conv2d<br/>[N*16, T, H, W]"]
        Decoder["Decoder3d<br/>逐帧处理"]
        Output["视频帧<br/>[N*3, T*4, H*8, W*8]"]
    end

    subgraph PostProc["输出处理"]
        Save["保存为 AVI/MP4"]
        Return["返回张量"]
    end

    Text --> Tokenize
    Tokenize --> Embed
    Embed --> TextModel
    TextModel --> TextOut

    Noise --> PatchEmb
    Params --> PatchEmb
    TextOut --> Blocks

    PatchEmb --> TimeEmb
    TimeEmb --> PosEnc
    PosEnc --> Blocks
    Blocks --> Head
    Head --> Unpatch

    Unpatch --> Conv2
    Conv2 --> Decoder
    Decoder --> Output

    Output --> Save
    Output --> Return
```

## 4. I2V/TI2V (图像到视频) 流程差异

```mermaid
graph TD
    subgraph Input["输入准备"]
        Image["参考图像<br/>[N, 3, 224, 224]"]
        Text["文本提示"]
        Noise["噪声初始化"]
    end

    subgraph ImageEnc["图像编码阶段"]
        CLIPVision["CLIPVisionModel<br/>图像特征提取"]
        ImageFeat["图像特征<br/>[N, 257, 1280]"]
        MLPProj["MLPProj<br/>特征投影"]
        ProjFeat["投影特征<br/>[N, 257, dim]"]
    end

    subgraph TextEnc["文本编码阶段"]
        TextModel["CLIPTextModel<br/>或 T5Model"]
        TextFeat["文本特征<br/>[N, 512, 4096]"]
    end

    subgraph Fusion["特征融合"]
        Concat["拼接特征<br/>[N, 257+512, dim]"]
    end

    subgraph TransformerDiT["Transformer DiT 推理<br/>使用 I2V/TI2V Cross-Attention"]
        Blocks["30-40 层 Transformer Blocks<br/>WanI2VCrossAttention<br/>双路 attention"]
        Output["输出<br/>[N*16, T, H, W]"]
    end

    subgraph VAEDec["VAE 解码阶段"]
        Decoder["Decoder3d"]
        Video["视频帧<br/>[N*3, T*4, H*8, W*8]"]
    end

    Image --> CLIPVision
    CLIPVision --> ImageFeat
    ImageFeat --> MLPProj
    MLPProj --> ProjFeat

    Text --> TextModel
    TextModel --> TextFeat

    ProjFeat --> Concat
    TextFeat --> Concat

    Concat --> Blocks
    Noise --> Blocks

    Blocks --> Output
    Output --> Decoder
    Decoder --> Video
```

## 5. WanAttentionBlock 内部流程

```mermaid
graph TD
    subgraph Input["输入"]
        X["x: [N, seq_len, dim]"]
        Context["context: [N, ctx_len, dim]"]
        Time["time_emb: [N, dim]"]
    end

    subgraph SelfAttn["Self-Attention<br/>with RoPE"]
        QKV["生成 Q, K, V"]
        RoPE["应用 RoPE<br/>旋转位置编码"]
        Attn["Attention<br/>softmax(QK^T/√d)V"]
        SelfOut["Self-Attn 输出"]
    end

    subgraph CrossAttn["Cross-Attention"]
        CrossQKV["生成 Q, K, V<br/>K,V 来自 context"]
        CrossAttn["Cross-Attention<br/>softmax(QK^T/√d)V"]
        CrossOut["Cross-Attn 输出"]
    end

    subgraph FFN["FFN<br/>Linear + GELU + Linear"]
        Linear1["Linear(dim → 4*dim)"]
        GELU["GELU 激活"]
        Linear2["Linear(4*dim → dim)"]
        FFNOut["FFN 输出"]
    end

    subgraph Residual["残差连接"]
        Add1["x + Self-Attn"]
        Norm1["LayerNorm"]
        Add2["Norm1 + Cross-Attn"]
        Norm2["LayerNorm"]
        Add3["Norm2 + FFN"]
        Out["最终输出"]
    end

    X --> QKV
    QKV --> RoPE
    RoPE --> Attn
    Attn --> SelfOut

    Context --> CrossQKV
    CrossQKV --> CrossAttn
    CrossAttn --> CrossOut

    Time --> FFN

    SelfOut --> Add1
    X --> Add1
    Add1 --> Norm1

    CrossOut --> Add2
    Norm1 --> Add2
    Add2 --> Norm2

    FFNOut --> Add3
    Norm2 --> Add3
    Add3 --> Out

    Linear1 --> GELU
    GELU --> Linear2
    Linear2 --> FFNOut
```

## 6. VAE 解码流程

```mermaid
graph TD
    subgraph Input["输入"]
        Z["潜在表示<br/>z: [N*16, T, H, W]"]
    end

    subgraph Conv["初始卷积"]
        Conv2["Conv2d<br/>z → [N*16, T, H, W]"]
    end

    subgraph Decoder3d["Decoder3d<br/>逐帧处理"]
        CausalConv["CausalConv3d<br/>因果卷积"]
        MiddleBlocks["Middle Blocks<br/>ResidualBlock + AttentionBlock"]
        Upsamples["Upsamples<br/>Up_ResidualBlock with Resample"]
        Head["Head<br/>RMSNorm + SiLU + CausalConv3d"]
    end

    subgraph Output["输出"]
        Video["视频帧<br/>[N*3, T*4, H*8, W*8]<br/>RGB 格式"]
    end

    Z --> Conv2
    Conv2 --> CausalConv
    CausalConv --> MiddleBlocks
    MiddleBlocks --> Upsamples
    Upsamples --> Head
    Head --> Video
```

## 7. 模型注册机制流程

```mermaid
graph LR
    subgraph Compile["编译时"]
        Macro["REGISTER_MODEL_FACTORY<br/>宏展开"]
        Lambda["工厂 Lambda 函数<br/>std::make_unique<T>"]
        Static["静态初始化器<br/>extern C DCE Guard"]
    end

    subgraph Runtime["运行时"]
        Init["程序启动"]
        Register["静态初始化器执行"]
        Map["工厂函数注册到 map"]
        Registry["ModelRegistry<br/>单例"]
    end

    subgraph Usage["使用"]
        Create["create<T><br/>version_string"]
        Lookup["查找工厂函数"]
        Call["调用工厂 Lambda"]
        Instance["返回模型实例"]
    end

    Macro --> Lambda
    Lambda --> Static
    Static --> Init
    Init --> Register
    Register --> Map
    Map --> Registry

    Create --> Lookup
    Lookup --> Registry
    Registry --> Call
    Call --> Instance
```

## 8. 张量形状变换链 (T2V)

```mermaid
graph TD
    subgraph TextPath["文本路径"]
        T1["文本提示<br/>string"]
        T2["Tokenize<br/>→ [1, 77]"]
        T3["Embedding<br/>→ [1, 77, 768]"]
        T4["CLIPTextModel/T5<br/>→ [1, 512, 4096]"]
    end

    subgraph NoisePath["噪声路径"]
        N1["x ~ N(0,1)<br/>[N*16, T, H, W]"]
    end

    subgraph TransformerPath["Transformer 路径"]
        TR1["Patch Embedding<br/>[N*16, T, H, W]<br/>→ [N, t_len*h_len*w_len, 1536]"]
        TR2["30-40 Transformer Blocks<br/>→ [N, t_len*h_len*w_len, 1536]"]
        TR3["Head Output<br/>→ [N, t_len*h_len*w_len, 16]"]
        TR4["Unpatchify<br/>→ [N*16, T, H, W]"]
    end

    subgraph VAEPath["VAE 路径"]
        V1["Conv2d<br/>[N*16, T, H, W]"]
        V2["Decoder3d<br/>→ [N*3, T*4, H*8, W*8]"]
    end

    T1 --> T2
    T2 --> T3
    T3 --> T4

    N1 --> TR1
    T4 --> TR2
    TR1 --> TR2
    TR2 --> TR3
    TR3 --> TR4

    TR4 --> V1
    V1 --> V2
```

## 9. 计算图构建与执行流程

```mermaid
sequenceDiagram
    participant App as 应用代码
    participant Runner as GGMLRunner
    participant Graph as ggml_cgraph
    participant Compute as 计算引擎
    participant Backend as CPU/GPU

    App->>Runner: compute(inputs, outputs)
    Runner->>Runner: build_graph(inputs, outputs)
    Runner->>Graph: 创建计算图
    Runner->>Graph: 添加操作节点
    Runner->>Graph: 设置输入/输出
    Graph-->>Runner: 图构建完成

    Runner->>Compute: 执行计算图
    Compute->>Backend: 分配张量内存
    Compute->>Backend: 执行操作序列
    Backend->>Backend: 张量计算
    Backend-->>Compute: 计算完成
    Compute-->>Runner: 结果返回

    Runner-->>App: 输出张量
```

## 10. 错误处理流程

```mermaid
graph TD
    subgraph Error["错误检测"]
        E1["Unknown model version"]
        E2["Tensor shape mismatch"]
        E3["Out of memory"]
        E4["Invalid dtype"]
        E5["Computation failed"]
    end

    subgraph Handling["错误处理"]
        H1["检查 model_factory.cpp<br/>中的注册"]
        H2["验证输入张量维度"]
        H3["增加 ggml_init_params<br/>大小"]
        H4["使用 F32/F16/I32/I64"]
        H5["检查计算图构建"]
    end

    subgraph Recovery["恢复策略"]
        R1["重新加载模型"]
        R2["调整输入形状"]
        R3["分批处理"]
        R4["转换数据类型"]
        R5["重新构建计算图"]
    end

    E1 --> H1
    E2 --> H2
    E3 --> H3
    E4 --> H4
    E5 --> H5

    H1 --> R1
    H2 --> R2
    H3 --> R3
    H4 --> R4
    H5 --> R5
```

## 11. 内存管理流程

```mermaid
graph TD
    subgraph Allocation["内存分配"]
        A1["ggml_init_params<br/>初始化上下文"]
        A2["alloc_params_buffer<br/>分配参数缓冲区"]
        A3["build_graph<br/>分配计算图内存"]
    end

    subgraph Usage["内存使用"]
        U1["参数张量<br/>权重数据"]
        U2["激活张量<br/>中间结果"]
        U3["梯度张量<br/>反向传播"]
    end

    subgraph Optimization["内存优化"]
        O1["张量 Offload<br/>CPU ↔ GPU"]
        O2["计算图重用<br/>相同形状"]
        O3["融合操作<br/>减少中间张量"]
    end

    subgraph Cleanup["内存清理"]
        C1["ggml_free<br/>释放上下文"]
        C2["RAII 自动清理<br/>析构函数"]
    end

    A1 --> U1
    A2 --> U2
    A3 --> U3

    U1 --> O1
    U2 --> O2
    U3 --> O3

    O1 --> C1
    O2 --> C2
    O3 --> C1
```

## 12. 完整推理流程 (端到端)

```mermaid
graph TD
    subgraph Setup["初始化"]
        S1["创建后端"]
        S2["加载文本编码器"]
        S3["加载 Transformer"]
        S4["加载 VAE 解码器"]
    end

    subgraph Inference["推理"]
        I1["文本编码"]
        I2["噪声初始化"]
        I3["Transformer 推理<br/>多步去噪"]
        I4["VAE 解码"]
    end

    subgraph Output["输出"]
        O1["视频帧处理"]
        O2["保存为文件"]
        O3["返回结果"]
    end

    subgraph Cleanup["清理"]
        C1["释放内存"]
        C2["关闭后端"]
    end

    S1 --> S2
    S2 --> S3
    S3 --> S4
    S4 --> I1

    I1 --> I2
    I2 --> I3
    I3 --> I4

    I4 --> O1
    O1 --> O2
    O2 --> O3

    O3 --> C1
    C1 --> C2
```

---

## 图表说明

### 1. 系统架构流程图
展示了整个系统的分层结构，从应用层到底层计算库的完整调用链。

### 2. 模型加载流程图
详细展示了模型从创建到初始化的完整过程，包括工厂模式的应用。

### 3. T2V 推理流程图
展示了文本到视频的完整推理过程，包括文本编码、Transformer 推理和 VAE 解码三个主要阶段。

### 4. I2V/TI2V 流程差异
展示了图像到视频的推理过程，与 T2V 的主要差异在于额外的图像编码和特征融合阶段。

### 5. WanAttentionBlock 内部流程
详细展示了单个注意力块的内部结构，包括自注意力、交叉注意力和 FFN 三个子模块。

### 6. VAE 解码流程
展示了 VAE 解码器的内部结构，包括因果卷积、中间块、上采样和头部输出。

### 7. 模型注册机制流程
展示了编译时和运行时的模型注册过程，以及工厂模式的实现。

### 8. 张量形状变换链
展示了从输入到输出的完整张量形状变换过程。

### 9. 计算图构建与执行流程
展示了 GGML 计算图的构建和执行过程。

### 10. 错误处理流程
展示了常见错误的检测、处理和恢复策略。

### 11. 内存管理流程
展示了内存的分配、使用、优化和清理过程。

### 12. 完整推理流程
展示了从初始化到输出的完整端到端推理过程。

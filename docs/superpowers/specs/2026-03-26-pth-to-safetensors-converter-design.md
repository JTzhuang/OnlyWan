# 设计文档：.pth 到 .safetensors 转换工具

该文档定义了一个用于将 PyTorch `.pth` 模型权重文件转换为 `safetensors` 格式的 Python 脚本。

## 1. 目标
- 支持单个 `.pth` 文件的转换。
- 支持全局数据类型（DType）设置：`float16`、`bfloat16` 或 `float32`。
- 提供简单的命令行界面（CLI）。
- 确保转换过程中的内存效率。

## 2. 系统架构

### 2.1 依赖项
- `torch`: 用于加载和转换 PyTorch 张量。
- `safetensors`: 用于将张量保存为高效、安全的格式。
- `argparse`: 用于处理命令行参数。

### 2.2 核心流程
1. **参数解析**: 获取输入/输出路径、目标 `dtype` 和 `device`。
2. **权重加载**:
   - 使用 `torch.load` 载入权重。
   - 如果是完整检查点，仅提取 `state_dict`。
3. **数据转换**:
   - 遍历张量。
   - 过滤掉非张量数据。
   - 对所有浮点数张量应用 `to(target_dtype)`。
4. **保存**:
   - 使用 `safetensors.torch.save_file` 进行序列化。

## 3. CLI 接口
```bash
python scripts/pth_to_safetensors.py <input.pth> <output.safetensors> --dtype [fp16|bf16|fp32]
```

## 4. 关键考虑因素
- **内存优化**: 默认将权重加载到 CPU，并尽可能在转换后释放原权重引用。
- **安全性**: 使用最新的 PyTorch `weights_only=True` 选项（如果版本支持）。
- **错误处理**: 处理文件不存在、CUDA 不支持 `bf16` 等常见问题。

## 5. 验收标准
- 成功将 `.pth` 转换为 `.safetensors`。
- 生成的 `.safetensors` 可以被 `safetensors.torch.load_file` 正确加载。
- 权重值在指定精度下保持一致。

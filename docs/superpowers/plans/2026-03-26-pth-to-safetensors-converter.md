# .pth 到 .safetensors 转换工具实施计划

> **对于智能代理：** 需要子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 按任务逐步实施此计划。步骤使用复选框 (`- [ ]`) 语法进行跟踪。

**目标：** 实现一个 Python 脚本，将 PyTorch `.pth` 权重文件转换为 `safetensors` 格式，并支持全局数据类型设置。

**架构：** 该脚本采用 CLI 模式，使用 `argparse` 处理输入，使用 `torch` 加载权重并进行类型转换，最后使用 `safetensors` 进行保存。设计注重内存效率，默认在 CPU 上操作，并支持 `weights_only=True` 安全加载。

**技术栈：** Python, PyTorch, Safetensors, Argparse

---

### 任务 1：创建测试用例以验证基本转换

**文件：**
- 创建：`tests/scripts/test_pth_to_safetensors.py`

- [ ] **步骤 1：编写基础测试代码**

编写测试，创建一个包含随机权重的 `.pth` 文件，运行脚本进行转换，并验证生成的 `.safetensors` 文件。

```python
import torch
import os
import subprocess
from safetensors.torch import load_file

def test_conversion_basic():
    input_pth = "test_model.pth"
    output_st = "test_model.safetensors"

    # 创建模拟权重
    weights = {"layer1.weight": torch.randn(10, 10)}
    torch.save(weights, input_pth)

    try:
        # 运行转换脚本 (预期此时会失败，因为脚本尚未创建)
        subprocess.run(["python3", "scripts/pth_to_safetensors.py", input_pth, output_st, "--dtype", "fp32"], check=True)

        # 验证结果
        loaded = load_file(output_st)
        assert torch.allclose(weights["layer1.weight"], loaded["layer1.weight"])
    finally:
        if os.path.exists(input_pth): os.remove(input_pth)
        if os.path.exists(output_st): os.remove(output_st)
```

- [ ] **步骤 2：运行测试并验证失败**

运行：`pytest tests/scripts/test_pth_to_safetensors.py`
预期：失败（找不到脚本）

- [ ] **步骤 3：提交测试用例**

```bash
git add tests/scripts/test_pth_to_safetensors.py
git commit -m "test: add basic conversion test case"
```

---

### 任务 2：实现核心转换逻辑和 CLI

**文件：**
- 创建：`scripts/pth_to_safetensors.py`

- [ ] **步骤 1：实现脚本骨架和参数解析**

```python
import argparse
import torch
from safetensors.torch import save_file

def main():
    parser = argparse.ArgumentParser(description="Convert .pth to .safetensors")
    parser.add_argument("input", help="Path to input .pth file")
    parser.add_argument("output", help="Path to output .safetensors file")
    parser.add_argument("--dtype", choices=["fp16", "bf16", "fp32"], default="fp32")
    args = parser.parse_args()
    # ... 实现加载和转换 ...
```

- [ ] **步骤 2：实现权重加载和 DType 转换**

```python
    dtype_map = {
        "fp16": torch.float16,
        "bf16": torch.bfloat16,
        "fp32": torch.float32
    }
    target_dtype = dtype_map[args.dtype]

    print(f"Loading {args.input}...")
    state_dict = torch.load(args.input, map_location="cpu", weights_only=True)

    # 如果是完整 checkpoint，提取 state_dict
    if "state_dict" in state_dict:
        state_dict = state_dict["state_dict"]

    converted_dict = {}
    for k, v in state_dict.items():
        if isinstance(v, torch.Tensor):
            if v.is_floating_point():
                v = v.to(target_dtype)
            converted_dict[k] = v

    print(f"Saving to {args.output}...")
    save_file(converted_dict, args.output)
```

- [ ] **步骤 3：运行测试验证通过**

运行：`pytest tests/scripts/test_pth_to_safetensors.py`
预期：通过

- [ ] **步骤 4：提交实现代码**

```bash
git add scripts/pth_to_safetensors.py
git commit -m "feat: implement pth to safetensors conversion script"
```

---

### 任务 3：增加 DType 转换专项测试

**文件：**
- 修改：`tests/scripts/test_pth_to_safetensors.py`

- [ ] **步骤 1：编写 DType 验证测试**

```python
def test_conversion_dtype():
    input_pth = "test_dtype.pth"
    output_st = "test_dtype.safetensors"
    weights = {"w": torch.randn(5, 5).to(torch.float32)}
    torch.save(weights, input_pth)

    try:
        subprocess.run(["python3", "scripts/pth_to_safetensors.py", input_pth, output_st, "--dtype", "fp16"], check=True)
        loaded = load_file(output_st)
        assert loaded["w"].dtype == torch.float16
    finally:
        if os.path.exists(input_pth): os.remove(input_pth)
        if os.path.exists(output_st): os.remove(output_st)
```

- [ ] **步骤 2：运行并验证**

运行：`pytest tests/scripts/test_pth_to_safetensors.py`
预期：全部通过

- [ ] **步骤 3：提交测试更新**

```bash
git add tests/scripts/test_pth_to_safetensors.py
git commit -m "test: add dtype specific verification"
```

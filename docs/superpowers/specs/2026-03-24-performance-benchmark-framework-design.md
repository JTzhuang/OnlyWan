# 性能基准测试框架设计文档

**日期**: 2026-03-24
**作者**: Claude Code
**版本**: 1.0
**状态**: 设计中

---

## 概述

为 OnlyWan (WAN 视频生成库) 创建一个详细的性能基准测试框架，支持：
- 多分辨率、多step数的参数组合测试
- 不同数量GPU（1卡、2卡、4卡等）的性能对比
- 多格式输出（JSON、CSV、HTML）
- 性能基线管理与对比

## 目标与约束

### 目标
1. **性能优化支持**: 识别瓶颈，验证优化效果
2. **回归测试**: 发布前验证性能未下降
3. **CI/CD集成**: 自动化基准测试，生成报告
4. **数据分析**: 结构化输出，便于后续处理
5. **可视化展示**: 图表化呈现性能指标

### 约束
- Python 3.8+ 环境
- 依赖: argparse、json、csv、subprocess、pyyaml（可选）
- 无重量级框架依赖（pandas、matplotlib等）
- 支持 Linux + CUDA 环境

## 架构设计

### 目录结构

```
scripts/
├── benchmark_detailed.py              # 核心基准测试引擎
├── baseline_compare.py                # 基线对比工具
├── configs/
│   ├── benchmark_default.yaml         # 默认配置
│   ├── benchmark_quick.yaml           # 快速验证配置
│   └── benchmark_comprehensive.yaml   # 全面测试配置
└── templates/
    └── report_template.html           # HTML报告模板
```

### 核心组件

#### 1. benchmark_detailed.py

**职责**:
- 加载和解析配置文件（YAML/JSON）
- CLI 参数处理与优先级覆盖
- 生成参数组合（笛卡尔积）
- 顺序/并行执行基准测试
- 收集多维度性能指标
- 生成输出文件（JSON/CSV/HTML）

**主要函数**:
```python
def parse_config_and_args() -> dict
    # 加载配置文件，CLI参数覆盖

def generate_test_combinations(config: dict) -> List[dict]
    # 笛卡尔积组合参数

def run_single_benchmark(test_case: dict, cli_path: str, model_path: str) -> dict
    # 执行单个基准测试

def collect_system_info() -> dict
    # 收集GPU/CUDA/驱动版本等环境信息

def collect_gpu_metrics(gpu_ids: List[int]) -> dict
    # 使用nvidia-smi收集显存等指标

def generate_outputs(results: List[dict], config: dict) -> None
    # 生成JSON/CSV/HTML输出
```

#### 2. baseline_compare.py

**职责**:
- 加载两个基准测试结果（JSON）
- 对比性能差异
- 生成对比HTML报告
- 支持阈值告警（超过X%则高亮）

**主要函数**:
```python
def load_benchmark_results(filepath: str) -> dict
    # 加载JSON结果

def compute_deltas(baseline: dict, current: dict) -> dict
    # 计算相对差异

def generate_comparison_report(baseline: dict, current: dict, output_path: str) -> None
    # 生成对比HTML报告
```

### 配置文件结构

#### benchmark_default.yaml

```yaml
# 基准测试配置

test_mode: "standard"  # quick | standard | comprehensive | custom

test_cases:
  - name: "standard_video_generation"
    description: "标准视频生成测试"
    resolutions:
      - "512x512"
      - "768x768"
      - "1024x576"
    steps:
      - 20
      - 50
    prompts:
      - "A cat playing with a ball"
      - "An astronaut on the moon"
    num_frames: 16  # 生成帧数

gpu_config:
  auto_detect: true          # 自动检测可用GPU
  min_required: 1            # 最少需要GPU数
  max_gpus: null             # 最多使用GPU数（null表示不限制）
  configurations: [1, 2]     # 测试1卡、2卡配置
  prefer_gpus: [0, 1, 2, 3]  # 优先使用这些GPU ID

execution:
  mode: "sequential"         # sequential | parallel
  timeout_per_test: 600      # 单个测试超时（秒）
  retry_on_failure: 0        # 失败重试次数

output:
  format: ["json", "csv", "html"]  # 输出格式
  save_dir: "./benchmark_results"  # 结果保存目录
  auto_name: true                  # 自动按日期/时间命名
  include_raw_output: false        # 是否包含wan-cli原始输出

metrics:
  collect_memory: true       # 收集显存信息
  collect_comm_bandwidth: true  # 估算通讯带宽
  collect_power: false       # 收集功耗（需NVIDIA Power Monitoring）
```

#### benchmark_quick.yaml

```yaml
test_mode: "quick"

test_cases:
  - name: "quick_verification"
    resolutions: ["512x512"]
    steps: [10]
    prompts: ["A cat"]

gpu_config:
  auto_detect: true
  configurations: [1]

execution:
  mode: "sequential"
  timeout_per_test: 300

output:
  format: ["json", "html"]
```

#### benchmark_comprehensive.yaml

```yaml
test_mode: "comprehensive"

test_cases:
  - name: "full_resolution_steps_matrix"
    resolutions: ["256x256", "384x384", "512x512", "768x768", "1024x576", "1280x720"]
    steps: [10, 20, 50, 100]
    prompts:
      - "A cat playing with a ball"
      - "An astronaut floating in space"
      - "A sunset over mountains"

gpu_config:
  auto_detect: true
  configurations: [1, 2, 4]
  prefer_gpus: [0, 1, 2, 3]

execution:
  mode: "sequential"
  timeout_per_test: 900

output:
  format: ["json", "csv", "html"]
```

### 数据结构

#### 单个测试结果

```json
{
  "test_id": "20260324_093045_test_001",
  "timestamp": "2026-03-24T09:30:45Z",
  "config": {
    "resolution": "512x512",
    "steps": 20,
    "prompt": "A cat playing with a ball",
    "num_frames": 16,
    "num_gpus": 2,
    "gpu_ids": [0, 1]
  },
  "metrics": {
    "total_time_seconds": 125.43,
    "step_latency_ms": 6.27,
    "throughput_steps_per_second": 0.16,
    "first_frame_latency_ms": 2500.0,
    "memory_usage_mb": {
      "0": 8500,
      "1": 7200
    },
    "peak_memory_mb": 8500,
    "avg_memory_mb": 7850,
    "comm_bandwidth_estimated_mbps": 450.0
  },
  "system_info": {
    "timestamp": "2026-03-24T09:30:45Z",
    "cuda_version": "12.0",
    "driver_version": "535.104.05",
    "gpu_names": ["NVIDIA A100", "NVIDIA A100"],
    "python_version": "3.10.0",
    "hostname": "gpu-server-01"
  },
  "status": "success",
  "error_message": null,
  "raw_output": "..."
}
```

#### 聚合结果

```json
{
  "benchmark_session_id": "20260324_093045",
  "config": {
    "test_mode": "standard",
    "test_cases": [...],
    "gpu_config": {...}
  },
  "summary": {
    "total_tests": 12,
    "passed_tests": 12,
    "failed_tests": 0,
    "duration_seconds": 1500.0
  },
  "results": [
    { "test_id": "...", ... },
    { "test_id": "...", ... }
  ]
}
```

### HTML 报告结构

#### 报告模板 (report_template.html)

关键部分：
1. **头部信息**: 测试会话ID、时间戳、运行环境
2. **概览表**: 配置参数 + 关键指标总览
3. **性能分析**:
   - 延迟 vs 分辨率曲线图（SVG）
   - 延迟 vs step数曲线图
   - GPU数 vs 加速比曲线图
4. **显存分析**: GPU分布柱状图
5. **详细结果表**: 所有测试用例的完整数据
6. **对比视图**（可选）: 与基线的差异高亮

## 使用场景

### 场景1: 开发中的快速验证

```bash
# 使用快速配置，1-2分钟完成
python scripts/benchmark_detailed.py \
  --config scripts/configs/benchmark_quick.yaml \
  --cli ./build_cuda/bin/wan-cli \
  --model ./models/wan2.2.gguf

# 输出: ./benchmark_results/2026-03-24/quick_test_*.{json,html}
```

### 场景2: 版本发布前全面测试

```bash
# CI/CD 流程中执行
python scripts/benchmark_detailed.py \
  --config scripts/configs/benchmark_comprehensive.yaml \
  --cli ./build_cuda/bin/wan-cli \
  --model ./models/wan2.2.gguf \
  --mode ci \
  --output-dir ./ci_artifacts/benchmarks

# 生成报告，失败时退出非零状态
```

### 场景3: 性能优化效果验证

```bash
# 执行当前版本测试
python scripts/benchmark_detailed.py \
  --config scripts/configs/benchmark_default.yaml \
  --cli ./build_cuda/bin/wan-cli \
  --model ./models/wan2.2.gguf \
  --output-dir ./benchmark_results/optimization_test

# 与基线对比
python scripts/baseline_compare.py \
  --baseline ./benchmark_results/baseline/baseline_v1.0.json \
  --current ./benchmark_results/optimization_test/default_test_*.json \
  --output-dir ./benchmark_results/comparison \
  --threshold 5.0

# 输出对比HTML报告
```

### 场景4: 自定义参数测试

```bash
# CLI 参数优先级覆盖配置文件
python scripts/benchmark_detailed.py \
  --config scripts/configs/benchmark_default.yaml \
  --cli ./build_cuda/bin/wan-cli \
  --model ./models/wan2.2.gguf \
  --resolutions "512x512,768x768" \
  --steps "20" \
  --gpus "0,1,2" \
  --format json csv html
```

## 数据持久化策略

### 目录结构

```
benchmark_results/
├── 2026-03-24/
│   ├── quick_test_093045.json
│   ├── quick_test_093045.csv
│   ├── quick_test_093045.html
│   ├── default_test_104530.json
│   ├── default_test_104530.html
│   └── ...
├── 2026-03-23/
│   └── ...
├── baseline/
│   ├── baseline_v1.0.json       # v1.0版本基线
│   ├── baseline_v1.1.json       # v1.1版本基线
│   └── ...
└── comparison/
    ├── v1.0_vs_v1.1_20260324.html
    └── ...
```

### 自动命名规则

```
{test_mode}_{timestamp}.{format}

例如:
quick_test_093045.json      # 快速测试，09:30:45 运行
default_test_104530.csv     # 默认测试，10:45:30 运行
comprehensive_test_150000.html  # 全面测试，15:00:00 运行
```

## 实现细节

### 参数组合生成

使用笛卡尔积生成所有参数组合：

```python
from itertools import product

resolutions = ["512x512", "768x768"]
steps = [20, 50]
prompts = ["A cat", "A dog"]

combinations = [
    {
        "resolution": r,
        "steps": s,
        "prompt": p
    }
    for r, s, p in product(resolutions, steps, prompts)
]

# 结果: 2 * 2 * 2 = 8 个测试用例
```

### GPU 智能分配

1. **自动检测**: 使用 `nvidia-smi --list-gpus` 检测可用GPU
2. **手动指定**: `--gpus 0,1,2` 直接指定GPU ID
3. **数量配置**: 同时测试 1卡、2卡、4卡 配置
4. **优先级**: 手动指定 > 数量配置 > 自动检测

### 性能指标收集

1. **延迟指标**:
   - `total_time`: wan-cli 报告的总耗时
   - `step_latency`: total_time / steps
   - `first_frame_latency`: 首帧出现的时间（从日志解析）

2. **吞吐指标**:
   - `throughput_steps_per_second`: steps / total_time
   - （可扩展: `throughput_frames_per_second`）

3. **显存指标**:
   - 使用 `nvidia-smi --query-gpu=memory.used,memory.total`
   - 采样周期: 0.5秒（在测试执行期间）
   - 记录: 峰值、平均值、各GPU分布

4. **通讯带宽** (多GPU场景):
   - 粗估: `(total_data_mb) / (estimated_comm_time)`
   - 注: 仅为近似值，精确值需NCCL profiling

### 错误处理与重试

```python
for test_case in test_combinations:
    for attempt in range(retry_count + 1):
        try:
            result = run_single_benchmark(test_case)
            if result['success']:
                results.append(result)
                break
        except Exception as e:
            if attempt == retry_count:
                results.append({
                    'status': 'failed',
                    'error': str(e),
                    'test_case': test_case
                })
```

## 依赖与环保考虑

### Python 依赖
- 标准库: argparse, json, csv, subprocess, time, re, sys, pathlib
- 可选: pyyaml (用于配置文件解析)

### 外部工具
- `nvidia-smi`: GPU信息查询（必需）
- `wan-cli`: 基准测试目标（必需）

### 环保建议
- 尽量使用 `sequential` 执行模式避免资源浪费
- 提供 `--dry-run` 选项预览测试计划而不执行

## 扩展点

1. **更多性能指标**: 功耗（nvidia-smi dmon）、L2缓存命中率
2. **分布式测试**: 跨机器运行基准测试，聚合结果
3. **自动告警**: 性能下降超过阈值时发送通知
4. **实时仪表板**: Web UI 展示实时基准测试进度
5. **性能趋势分析**: 多个版本的历史对比

## 验收标准

- ✓ 支持YAML配置文件加载
- ✓ CLI参数优先级覆盖配置
- ✓ 笛卡尔积生成参数组合
- ✓ 收集延迟、吞吐、显存、通讯带宽指标
- ✓ 生成JSON、CSV、HTML格式输出
- ✓ 自动按日期分类保存结果
- ✓ 基线对比工具生成对比报告
- ✓ 支持多GPU数量配置
- ✓ 错误处理不中断整个流程
- ✓ 清晰的命令行帮助和使用示例

---

## 修订历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | 2026-03-24 | 初始设计文档 |


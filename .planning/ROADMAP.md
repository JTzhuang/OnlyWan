# Roadmap: wan-cpp

**Project:** Standalone C++ Library for WAN Video Generation
**Created:** 2026-03-12

## Milestones

- ✅ **v1.0 MVP** — Phases 1-8 (shipped 2026-03-16)
- 🔄 **v1.1 模型格式扩展** — Phases 9-13 (in progress)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 1-8) — SHIPPED 2026-03-16</summary>

- [x] Phase 1: Foundation (1/1 plans) — completed 2026-03-12
- [x] Phase 2: Build System (1/1 plans) — completed 2026-03-12
- [x] Phase 3: Public API (1/1 plans) — completed 2026-03-15
- [x] Phase 4: Examples (1/1 plans) — completed 2026-03-15
- [x] Phase 5: Encoders (1/1 plans) — completed 2026-03-16
- [x] Phase 6: Fix Duplicate Symbols (1/1 plans) — completed 2026-03-16
- [x] Phase 7: Wire Core Model to API (3/3 plans) — completed 2026-03-16
- [x] Phase 8: Implement Generation + AVI Output (2/2 plans) — completed 2026-03-16

See `.planning/milestones/v1.0-ROADMAP.md` for full phase details.

</details>

### v1.1 模型格式扩展 (Phases 9-13)

- [x] **Phase 9: API Fixes + Vocab mmap** - 移除遗留 stub，接通 progress_cb，词汇表改为 mmap 加载 (completed 2026-03-16)
- [x] **Phase 10: Safetensors Runtime Loading** - 运行时直接加载 .safetensors 格式 WAN 模型 (completed 2026-03-17)
- [x] **Phase 11: Safetensors Conversion Tool** - 独立 CLI 工具将 safetensors 转换为 GGUF (completed 2026-03-17)
- [x] **Phase 12: Wire Vocab Dir to Public API** - 暴露 wan_set_vocab_dir，修复 T2V/I2V 词汇表加载 (completed 2026-03-17)
- [x] **Phase 13: Document wan-convert Sub-model Scope** - 明确 vae/t5/clip 转换类型的使用限制 (completed 2026-03-17)

## Phase Details

### Phase 9: API Fixes + Vocab mmap
**Goal**: v1.0 遗留问题全部修复，API 行为与文档一致
**Depends on**: Phase 8 (v1.0 complete)
**Requirements**: FIX-01, FIX-02, PERF-01
**Success Criteria** (what must be TRUE):
  1. 调用 `wan_generate_video_t2v` / `wan_generate_video_i2v` 实际执行生成，不再返回 WAN_ERROR_UNSUPPORTED
  2. 生成过程中 progress_cb 在每个 Euler 步骤触发，传入正确的 step/total 值
  3. 库编译时不再嵌入 ~85MB 词汇表头文件；词汇表从外部文件 mmap 加载
  4. 现有 T2V/I2V 生成结果与 v1.0 _ex 接口输出一致（无回归）
**Plans**: 2 plans
Plans:
- [x] 09-01-PLAN.md — Fix T2V/I2V stubs (FIX-01) + wire progress_cb into both Euler loops (FIX-02)
- [x] 09-02-PLAN.md — Replace embedded vocab arrays with mmap loading + WAN_EMBED_VOCAB CMake option (PERF-01)

### Phase 10: Safetensors Runtime Loading
**Goal**: 用户可直接用 .safetensors 文件调用 wan_load_model，无需预转换
**Depends on**: Phase 9
**Requirements**: SAFE-01
**Success Criteria** (what must be TRUE):
  1. `wan_load_model` 接受 .safetensors 路径，成功返回有效 WanModel 句柄
  2. 用 safetensors 加载的模型执行 T2V 生成，输出与 GGUF 加载结果等价
  3. 传入无效或损坏的 safetensors 文件时返回明确错误码，不崩溃
**Plans**: 1 plan
Plans:
- [x] 10-01-PLAN.md — Add safetensors dispatch branch to WanModel::load (SAFE-01)

### Phase 11: Safetensors Conversion Tool
**Goal**: 用户可将 WAN2.1/2.2 所有子模型从 safetensors 批量转换为 GGUF
**Depends on**: Phase 10
**Requirements**: SAFE-02, SAFE-03
**Success Criteria** (what must be TRUE):
  1. `wan-convert` CLI 可执行文件存在，`--help` 输出用法说明
  2. 转换 DiT、VAE、T5、CLIP 子模型各自的 safetensors 文件，生成可被 wan_load_model 加载的 GGUF 文件
  3. 转换后的 GGUF 文件执行 T2V/I2V 生成，输出与原始 safetensors 直接加载结果一致
**Plans**: 1 plan
Plans:
- [x] 11-01-PLAN.md — Extend save_to_gguf_file with metadata map + wan-convert CLI + CMake wiring (SAFE-02, SAFE-03)

### Phase 12: Wire Vocab Dir to Public API
**Goal**: T2V/I2V 生成在 WAN_EMBED_VOCAB=OFF 时正常工作，词汇表目录通过公共 API 设置
**Depends on**: Phase 11
**Requirements**: PERF-01, ENCODER-01, ENCODER-02, API-03, API-04
**Success Criteria** (what must be TRUE):
  1. `wan.h` 声明 `wan_set_vocab_dir(const char* dir)` 函数
  2. `wan-cli` 支持 `--vocab-dir` 参数，调用 `wan_set_vocab_dir` 后 T2V/I2V 生成成功
  3. 使用 `WAN_EMBED_VOCAB=OFF` 构建时，提供词汇表目录后生成不返回 `WAN_ERROR_INVALID_ARGUMENT`
**Plans**: 1 plan
Plans:
- [ ] 12-01-PLAN.md — Add vocab accessors + wan_set_vocab_dir public API + --vocab-dir CLI arg (PERF-01, ENCODER-01, ENCODER-02, API-03, API-04)

### Phase 13: Document wan-convert Sub-model Scope
**Goal**: 用户清楚了解 wan-convert 各 --type 值的适用范围，SAFE-03 限制有文档说明
**Depends on**: Phase 12
**Requirements**: SAFE-03
**Success Criteria** (what must be TRUE):
  1. `wan-convert --help` 输出说明哪些 `--type` 值（dit-t2v/dit-i2v/dit-ti2v）产生可被 `wan_load_model` 加载的文件
  2. README 或 examples/convert/ 文档说明 vae/t5/clip 类型为未来多文件加载预留
  3. REQUIREMENTS.md SAFE-03 追踪性更新，反映当前部分满足状态及限制
**Plans**: 1 plan
Plans:
- [ ] 13-01-PLAN.md — Annotate print_usage() + create examples/convert/README.md + verify SAFE-03 traceability (SAFE-03)

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Foundation | v1.0 | 1/1 | Complete | 2026-03-12 |
| 2. Build System | v1.0 | 1/1 | Complete | 2026-03-12 |
| 3. Public API | v1.0 | 1/1 | Complete | 2026-03-15 |
| 4. Examples | v1.0 | 1/1 | Complete | 2026-03-15 |
| 5. Encoders | v1.0 | 1/1 | Complete | 2026-03-16 |
| 6. Fix Duplicate Symbols | v1.0 | 1/1 | Complete | 2026-03-16 |
| 7. Wire Core Model to API | v1.0 | 3/3 | Complete | 2026-03-16 |
| 8. Implement Generation + AVI Output | v1.0 | 2/2 | Complete | 2026-03-16 |
| 9. API Fixes + Vocab mmap | v1.1 | 2/2 | Complete | 2026-03-16 |
| 10. Safetensors Runtime Loading | v1.1 | 1/1 | Complete | 2026-03-17 |
| 11. Safetensors Conversion Tool | v1.1 | 1/1 | Complete | 2026-03-17 |
| 12. Wire Vocab Dir to Public API | v1.1 | 1/1 | Complete | 2026-03-17 |
| 13. Document wan-convert Sub-model Scope | v1.1 | 1/1 | Complete | 2026-03-17 |
| 14. 性能优化 - CUDA Graph 和算子融合 | 2/2 | Complete   | 2026-03-17 | - |

### Phase 14: 性能优化 - CUDA Graph 和算子融合

**Goal:** 实现 5 个 Quick Wins 优化（CG-01、OP-01、CG-02、OP-02、FUS-02），达成 2-5x 去噪循环加速和 10-20% 整体推理加速
**Requirements**: CG-01, OP-01, CG-02, OP-02, FUS-02
**Depends on:** Phase 13
**Success Criteria** (what must be TRUE):
  1. 去噪循环中 compute buffer 只分配一次，graph 结构跨步复用
  2. CUDA/Metal 后端自动启用 Flash Attention
  3. GGML_CUDA_USE_GRAPHS 编译标志在 CUDA 构建中自动定义
  4. RoPE PE 在去噪循环中只计算一次，后续步从缓存读取
  5. FFN 路径使用 inplace GELU 或融合的 linear_gelu
**Plans**: 2 plans

Plans:
- [ ] 14-01-PLAN.md — 缓冲区持久化 + Flash Attention 自动启用 + CUDA Graph 编译标志 (CG-01, OP-01, CG-02)
- [ ] 14-02-PLAN.md — RoPE PE GPU 化 + Linear+GELU 算子融合 (OP-02, FUS-02)

---
*Last updated: 2026-03-17 — Phase 14 planned*

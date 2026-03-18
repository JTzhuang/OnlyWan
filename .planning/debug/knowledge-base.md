# GSD Debug Knowledge Base

Resolved debug sessions. Used by `gsd-debugger` to surface known-pattern hypotheses at the start of new investigations.

---

## phase-15-compilation — Phase 15 multi-GPU implementation compilation errors
- **Date:** 2026-03-18
- **Error patterns:** compilation errors, ggml-backend-sched.h, No such file or directory, circular_y_enabled not declared, MultiGPUState not defined, ggml_backend_sched_new wrong arguments, ggml_backend_cuda_split_buffer_type not found, wan_batch_result_t not declared, wan_params_set_gpu_ids not declared
- **Root cause:** Phase 15 implementation had multiple API integration issues: (1) Incorrect header include - tried to include non-existent "ggml-backend-sched.h" instead of "ggml-cuda.h", (2) Wrong include guard - used WAN_USE_MULTI_GPU instead of GGML_USE_CUDA for CUDA header, (3) Wrong function signature - ggml_backend_cuda_get_device_description requires buffer and size parameters, (4) Type definition scope issue - wan_batch_result_t was inside WAN_USE_MULTI_GPU guard but used by stub function outside the guard, (5) CLI example used multi-GPU functions without guards. Additionally, WAN_USE_MULTI_GPU was disabled in CMakeLists.txt because the full Phase 15 multi-GPU implementation has deeper API mismatches that need separate fixing.
- **Fix:** (1) Changed include from "ggml-backend-sched.h" to "ggml-cuda.h" in wan-internal.hpp, (2) Changed include guard from WAN_USE_MULTI_GPU to GGML_USE_CUDA, (3) Fixed ggml_backend_cuda_get_device_description call to use buffer and size parameters, (4) Moved wan_batch_result_t typedef outside WAN_USE_MULTI_GPU guard in wan.h, (5) Added WAN_USE_MULTI_GPU guards around multi-GPU parameter calls in main.cpp, (6) Disabled WAN_USE_MULTI_GPU in CMakeLists.txt
- **Files changed:** src/ggml_extend.hpp, include/wan-cpp/wan-internal.hpp, src/api/wan-api.cpp, include/wan-cpp/wan.h, examples/cli/main.cpp, CMakeLists.txt
---


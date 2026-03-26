---
status: root_cause_found
trigger: "CUDA backend compiled (WAN_CUDA=ON) but runtime inference still on CPU"
created: 2026-03-26
updated: 2026-03-26
---

## Current Focus

ROOT CAUSE IDENTIFIED: Config backend field parsed but never used by inference code

## Symptoms

expected: "When backend is set to 'cuda' in config file, inference should run on CUDA device"
actual: "Inference runs on CPU despite WAN_CUDA=ON and setting backend='cuda' in config.json"
errors: []
reproduction: "User sets 'backend': 'cuda' in config.json, but inference remains CPU-bound"
started: "Unknown - config-driven design not fully implemented"

## Eliminated

None - root cause identified directly

## Evidence

### Evidence 1: Config System Supports Backend Field (parse layer)
- checked: "src/config_loader.hpp and src/config_loader.cpp"
- found: |
    Line 11 (hpp): struct WanLoadConfig { std::string backend; ... }
    Line 28 (cpp): cfg.backend = j.value("backend", "cpu")
  Config loader DOES parse backend from JSON file with default "cpu"
- implication: "Config file format supports backend setting; parsing works correctly"

### Evidence 2: Parsed Config Backend Is Never Used
- checked: "Entire codebase search for 'config.backend' and usage"
- found: "Zero grep matches for 'config.backend' - value is never read after initial parse"
- implication: "Parsed backend value from config is abandoned; dead code"

### Evidence 3: WanModelLoadResult Cannot Return Backend
- checked: "include/wan-cpp/wan-internal.hpp lines 77-86"
- found: |
    struct WanModelLoadResult {
        bool success;
        shared_ptr<WAN::WanRunner> wan_runner;
        shared_ptr<WAN::WanVAERunner> vae_runner;
        shared_ptr<T5Embedder> t5_embedder;
        shared_ptr<CLIPVisionModelProjectionRunner> clip_runner;
        string model_type;
        string error_message;
        string model_version;
    }
  No backend field present in result structure
- implication: "No mechanism to return parsed backend from WanModel::load()"

### Evidence 4: wan_load_model API Ignores Config Backend
- checked: "src/api/wan-api.cpp wan_load_model() function lines 353-440"
- found: |
    Line 396: ctx->backend_type = backend_type ? backend_type : "cpu"
             Uses PARAMETER only, never checks config
    Line 400: WanModelLoadResult result = Wan::WanModel::load(ctx->model_path)
             Loads config file but backend field not extracted
    Line 417: ctx->backend = WanBackendPtr(Wan::WanBackend::create(ctx->backend_type, n_threads, 0))
             Creates backend using ctx->backend_type from PARAMETER, not config
- implication: "Backend creation driven entirely by parameter, config ignored"

### Evidence 5: CLI Defaults to CPU When Backend Not Specified
- checked: "examples/cli/main.cpp argument handling"
- found: |
    Line 138: opts->backend = NULL (initialized to NULL)
    Line 456: wan_load_model(opts.config_path, opts.threads, opts.backend, &ctx)
             Passes opts.backend (NULL) as parameter
    Line 396 (wan-api.cpp): backend_type = NULL ? NULL : "cpu" => "cpu"
  NULL parameter defaults to "cpu"
- implication: "CLI ignores config backend; requires --backend flag to override"

## Root Cause Analysis

**The configuration system is architecturally incomplete:**

Data flow for backend selection:
1. ConfigLoader::load_config() successfully parses config.backend ✓
2. Parsed value stored in WanLoadConfig::backend ✓
3. WanModel::load() receives WanLoadConfig with backend value ✓
4. BUT: WanModelLoadResult struct has NO backend field ✗
5. Backend value CANNOT be returned from WanModel::load() ✗
6. wan_load_model() NEVER attempts to read backend from config ✗
7. Backend creation uses ONLY backend_type parameter ✗
8. Parameter defaults to "cpu" if not explicitly provided ✗

Result: Users setting "backend": "cuda" in config.json see NO EFFECT

## Architecture Issue

The config system has three separate concerns that don't connect:

| Layer | Status | Note |
|-------|--------|------|
| Parsing | �� Works | Config file supports backend field |
| Passing | ✗ Broken | WanModelLoadResult cannot return backend |
| Usage | ✗ Broken | wan_load_model() doesn't use config backend |

It's like a three-stage pipeline where the middle stage is disconnected.

## User Impact

User expectation: "I'll set backend in config.json and it will be used"
User reality: "Backend must be passed as CLI flag or API parameter"

Workarounds users must know about:
- CLI: Use `--backend cuda` flag (not in config)
- API direct: `wan_load_model(path, 0, "cuda", &ctx)`
- API with params: `wan_params_set_backend(params, "cuda")`

## Solution

Two approaches:

**Approach A: Accept Parameter-Driven Design (minimal effort)**
- Update CONFIG.md documentation
- Clarify that backend is NOT config-driven
- Backend must be passed as parameter
- Removes confusion but doesn't fulfill config system promise

**Approach B: Implement Config-Driven Backend (respects design)**
- Add backend field to WanModelLoadResult struct
- Return config.backend from WanModel::load()
- Modify wan_load_model(): read config.backend, use as default
- Allow parameter to override config backend
- Updates needed in:
  - include/wan-cpp/wan-internal.hpp (add field)
  - src/api/wan-api.cpp (extract and use backend)
  - examples/cli/main.cpp (respect config when flag not provided)

Approach B better aligns with stated config-driven design philosophy.

## Files Affected

- /src/config_loader.hpp - Defines WanLoadConfig.backend field
- /src/config_loader.cpp - Parses backend from JSON
- /include/wan-cpp/wan-internal.hpp - WanModelLoadResult (missing backend field)
- /src/api/wan-api.cpp - wan_load_model() doesn't extract/use backend
- /examples/cli/main.cpp - CLI passes backend as parameter only

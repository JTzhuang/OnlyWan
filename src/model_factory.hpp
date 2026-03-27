#ifndef __MODEL_FACTORY_HPP__
#define __MODEL_FACTORY_HPP__

#include "model_registry.hpp"

// WAN model headers (full includes for factory lambdas)
#include "clip.hpp"       // CLIPTextModelRunner, CLIPVersion enum
#include "t5.hpp"         // T5Runner
#include "wan.hpp"        // WAN::WanVAERunner, WAN::WanRunner, VERSION_WAN2, VERSION_WAN2_2_*
#include "ggml_extend.hpp"
#include "model.h"        // String2TensorStorage, SDVersion enum
#include "ggml-backend.h" // ggml_backend_t

// NOTE: flux.hpp is intentionally NOT included — it has been removed from src/.
// FluxRunner is out of scope for this phase.

#ifdef __cplusplus
extern "C" {
#endif

// Call this once from test main() or test setup to prevent linker DCE of model_factory.cpp
void wan_force_model_registrations();

#ifdef __cplusplus
}
#endif

#endif // __MODEL_FACTORY_HPP__

#include "model.h"
#include "util.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

// Sub-model type -> GGUF metadata mapping.
// Keys read by is_wan_gguf() in src/api/wan_loader.cpp:
//   general.model_type    -> "wan"
//   general.architecture  -> substring matched for T2V/I2V/TI2V
//   general.wan.version   -> "WAN2.1" or "WAN2.2"
struct SubModelInfo {
    const char* arch;
    const char* version;
};

static const std::map<std::string, SubModelInfo> SUBMODEL_META = {
    {"dit-t2v",  {"WAN-T2V",  "WAN2.1"}},
    {"dit-i2v",  {"WAN-I2V",  "WAN2.2"}},
    {"dit-ti2v", {"WAN-TI2V", "WAN2.2"}},
    {"vae",      {"WAN-VAE",  "WAN2.1"}},
    {"t5",       {"WAN-T5",   "WAN2.1"}},
    {"clip",     {"WAN-CLIP", "WAN2.2"}},
};

static void print_usage(const char* prog) {
    fprintf(stdout,
        "Usage: %s --input <file.safetensors> --output <file.gguf> --type <submodel> [--quant <type>]\n"
        "\n"
        "Options:\n"
        "  --input  <path>     Input safetensors file (required)\n"
        "  --output <path>     Output GGUF file (required)\n"
        "  --type   <submodel> Sub-model type (required):\n"
        "                        dit-t2v   WAN2.1 DiT text-to-video\n"
        "                        dit-i2v   WAN2.2 DiT image-to-video\n"
        "                        dit-ti2v  WAN2.2 DiT text+image-to-video\n"
        "                        vae       VAE encoder/decoder\n"
        "                        t5        T5 text encoder\n"
        "                        clip      CLIP image encoder\n"
        "  --quant  <type>     Quantisation type (default: f16)\n"
        "                        f32, f16, q8_0, q4_0, q4_1\n"
        "  --help              Show this message\n",
        prog);
}

int main(int argc, char** argv) {
    std::string input, output, submodel_type, quant = "f16";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            submodel_type = argv[++i];
        } else if (strcmp(argv[i], "--quant") == 0 && i + 1 < argc) {
            quant = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input.empty() || output.empty() || submodel_type.empty()) {
        fprintf(stderr, "error: --input, --output, and --type are required\n");
        print_usage(argv[0]);
        return 1;
    }

    auto meta_it = SUBMODEL_META.find(submodel_type);
    if (meta_it == SUBMODEL_META.end()) {
        fprintf(stderr, "error: unknown --type '%s'\n", submodel_type.c_str());
        fprintf(stderr, "valid types: dit-t2v, dit-i2v, dit-ti2v, vae, t5, clip\n");
        return 1;
    }

    // Map quant string to ggml_type
    ggml_type wtype = GGML_TYPE_F16;
    if (quant == "f32")       wtype = GGML_TYPE_F32;
    else if (quant == "f16")  wtype = GGML_TYPE_F16;
    else if (quant == "q8_0") wtype = GGML_TYPE_Q8_0;
    else if (quant == "q4_0") wtype = GGML_TYPE_Q4_0;
    else if (quant == "q4_1") wtype = GGML_TYPE_Q4_1;
    else {
        fprintf(stderr, "error: unknown --quant '%s'\n", quant.c_str());
        return 1;
    }

    LOG_INFO("loading %s", input.c_str());
    ModelLoader loader;
    if (!loader.init_from_file(input)) {
        LOG_ERROR("failed to load %s", input.c_str());
        return 1;
    }
    loader.convert_tensors_name();

    // Build metadata map — keys required by is_wan_gguf() in wan_loader.cpp
    const SubModelInfo& info = meta_it->second;
    std::map<std::string, std::string> metadata = {
        {"general.model_type",   "wan"},
        {"general.architecture", info.arch},
        {"general.wan.version",  info.version},
    };

    LOG_INFO("converting to %s (type=%s quant=%s)", output.c_str(), submodel_type.c_str(), quant.c_str());
    if (!loader.save_to_gguf_file(output, wtype, "", metadata)) {
        LOG_ERROR("conversion failed");
        return 1;
    }

    LOG_INFO("done: %s -> %s", input.c_str(), output.c_str());
    return 0;
}

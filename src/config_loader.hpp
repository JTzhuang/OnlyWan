#ifndef __CONFIG_LOADER_HPP__
#define __CONFIG_LOADER_HPP__

#include <string>
#include <vector>
#include "json.hpp"
#include "model.h"             // For SDVersion

struct WanLoadConfig {
    // Runtime environment
    std::string backend;
    int n_threads;
    std::vector<int> gpu_ids;

    // Model paths
    std::string transformer_path;
    std::string vae_path;
    std::string text_encoder_path;
    std::string clip_path;  // Optional

    // Architecture config file
    std::string wan_config_file;
};

struct WanArchConfig {
    // From wan_config.json
    std::string model_type;      // t2v, i2v, ti2v
    int dim;
    int num_heads;
    int num_layers;
    int in_dim;
    int out_dim;
    int text_len;
    float eps;
    int ffn_dim;
    int freq_dim;
};

class ConfigLoader {
public:
    /**
     * Load main configuration from JSON file
     * @param config_path Path to config.json
     * @return Parsed WanLoadConfig
     * @throws std::runtime_error if file not found or JSON is invalid
     */
    static WanLoadConfig load_config(const std::string& config_path);

    /**
     * Load architecture configuration from JSON file
     * @param config_path Path to wan_config.json
     * @return Parsed WanArchConfig
     * @throws std::runtime_error if file not found or JSON is invalid
     */
    static WanArchConfig load_arch_config(const std::string& config_path);

    /**
     * Validate that all required model paths exist
     * @param config WanLoadConfig to validate
     * @throws std::runtime_error if required files are missing
     */
    static void validate_required_files(const WanLoadConfig& config);
};

#endif // __CONFIG_LOADER_HPP__

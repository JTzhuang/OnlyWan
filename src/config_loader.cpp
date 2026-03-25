#include "config_loader.hpp"
#include "util.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

WanLoadConfig ConfigLoader::load_config(const std::string& config_path) {
    LOG_DEBUG("ConfigLoader::load_config: parsing %s", config_path.c_str());

    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }

    nlohmann::json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON parse error in " + config_path + ": " + e.what());
    }

    WanLoadConfig cfg;

    // Parse fields with defaults (backend, n_threads, gpu_ids)
    cfg.backend = j.value("backend", "cpu");
    cfg.n_threads = j.value("n_threads", 0);

    if (j.contains("gpu_ids") && j["gpu_ids"].is_array()) {
        cfg.gpu_ids = j["gpu_ids"].get<std::vector<int>>();
    }

    // Parse required model paths
    try {
        cfg.transformer_path = j.at("models").at("transformer_path").get<std::string>();
        cfg.vae_path = j.at("models").at("vae_path").get<std::string>();
        cfg.text_encoder_path = j.at("models").at("text_encoder_path").get<std::string>();
        cfg.wan_config_file = j.at("wan_config_file").get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Missing required field in config: " + std::string(e.what()));
    }

    // Parse optional clip_path
    if (j.contains("models") && j["models"].contains("clip_path")) {
        cfg.clip_path = j["models"]["clip_path"].get<std::string>();
    }

    LOG_DEBUG("ConfigLoader::load_config: parsed successfully");
    return cfg;
}

WanArchConfig ConfigLoader::load_arch_config(const std::string& config_path) {
    LOG_DEBUG("ConfigLoader::load_arch_config: parsing %s", config_path.c_str());

    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open arch config file: " + config_path);
    }

    nlohmann::json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON parse error in " + config_path + ": " + e.what());
    }

    WanArchConfig cfg;

    try {
        cfg.model_type = j.at("model_type").get<std::string>();
        cfg.dim = j.at("dim").get<int>();
        cfg.num_heads = j.at("num_heads").get<int>();
        cfg.num_layers = j.at("num_layers").get<int>();
        cfg.in_dim = j.at("in_dim").get<int>();
        cfg.out_dim = j.at("out_dim").get<int>();
        cfg.text_len = j.at("text_len").get<int>();
        cfg.eps = j.value("eps", 1e-6f);
        cfg.ffn_dim = j.at("ffn_dim").get<int>();
        cfg.freq_dim = j.at("freq_dim").get<int>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Missing required arch field: " + std::string(e.what()));
    }

    LOG_DEBUG("ConfigLoader::load_arch_config: parsed successfully, model_type=%s", cfg.model_type.c_str());
    return cfg;
}

void ConfigLoader::validate_required_files(const WanLoadConfig& config) {
    LOG_DEBUG("ConfigLoader::validate_required_files: validating required files");

    // Check required files
    std::vector<std::pair<std::string, std::string>> required = {
        {"transformer_path", config.transformer_path},
        {"vae_path", config.vae_path},
        {"text_encoder_path", config.text_encoder_path}
    };

    for (const auto& [name, path] : required) {
        if (!file_exists(path)) {
            throw std::runtime_error("Required model file not found: " + name + " = " + path);
        }
        LOG_DEBUG("ConfigLoader::validate_required_files: ✓ %s exists", name.c_str());
    }

    // Check wan_config_file
    if (!file_exists(config.wan_config_file)) {
        throw std::runtime_error("Architecture config file not found: " + config.wan_config_file);
    }
    LOG_DEBUG("ConfigLoader::validate_required_files: ✓ wan_config_file exists");

    // Optional: warn if clip_path is empty but might be needed for i2v/ti2v
    // (actual check is done in WanModel::load() after reading model_type)
}

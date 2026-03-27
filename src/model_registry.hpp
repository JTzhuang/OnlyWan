#ifndef __MODEL_REGISTRY_HPP__
#define __MODEL_REGISTRY_HPP__

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ggml-backend.h"
#include "model.h"

// Factory function signature for model creation
template <typename ModelType>
using FactoryFn = std::function<std::unique_ptr<ModelType>(
    ggml_backend_t backend,
    bool offload_params_to_cpu,
    const String2TensorStorage& tensor_map,
    const std::string& prefix)>;

class ModelRegistry {
public:
    static ModelRegistry* instance();

    template <typename ModelType>
    void register_factory(const std::string& version, FactoryFn<ModelType> factory) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = get_key<ModelType>(version);
        registry_[key] = [factory](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) -> std::unique_ptr<void, void (*)(void*)> {
            auto ptr = factory(backend, offload, map, prefix);
            return std::unique_ptr<void, void (*)(void*)>(ptr.release(), [](void* p) { delete static_cast<ModelType*>(p); });
        };
    }

    template <typename ModelType>
    std::unique_ptr<ModelType> create(const std::string& version,
                                      ggml_backend_t backend,
                                      bool offload_params_to_cpu,
                                      const String2TensorStorage& tensor_map,
                                      const std::string& prefix = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = get_key<ModelType>(version);
        auto it  = registry_.find(key);
        if (it == registry_.end()) {
            throw std::runtime_error("Unknown model version: " + version + " for requested type");
        }
        auto erased_ptr = it->second(backend, offload_params_to_cpu, tensor_map, prefix);
        return std::unique_ptr<ModelType>(static_cast<ModelType*>(erased_ptr.release()));
    }

    template <typename ModelType>
    bool has_version(const std::string& version) {
        std::lock_guard<std::mutex> lock(mutex_);
        return registry_.count(get_key<ModelType>(version)) > 0;
    }

private:
    ModelRegistry() = default;
    ~ModelRegistry() = default;

    template <typename ModelType>
    std::string get_key(const std::string& version) {
        // Simple type-name based key. In production, we might want something more robust
        // but for this project's scope, typeid or a manual name works.
        // We use the version string directly as a key, but we need to distinguish
        // between different ModelTypes if they share version strings.
        return std::string(typeid(ModelType).name()) + ":" + version;
    }

    using ErasedFactoryFn = std::function<std::unique_ptr<void, void (*)(void*)>(
        ggml_backend_t, bool, const String2TensorStorage&, const std::string&)>;

    std::map<std::string, ErasedFactoryFn> registry_;
    std::mutex mutex_;
};

#define REGISTER_MODEL_FACTORY(ModelType, VersionString, FactoryBody)           \
    namespace {                                                                 \
    struct Registrar_##ModelType##_##VersionString {                            \
        Registrar_##ModelType##_##VersionString() {                             \
            ModelRegistry::instance()->register_factory<ModelType>(             \
                VersionString,                                                  \
                FactoryBody);                                                   \
        }                                                                       \
    };                                                                          \
    static Registrar_##ModelType##_##VersionString global_registrar_##ModelType##_##VersionString; \
    }

#endif // __MODEL_REGISTRY_HPP__

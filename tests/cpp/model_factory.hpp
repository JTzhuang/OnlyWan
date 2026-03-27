#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>

#include "model.h"        // String2TensorStorage typedef (OrderedMap<string, TensorStorage>)
#include "ggml-backend.h" // ggml_backend_t

// Generic template factory for model version registration and creation.
//
// Usage:
//   ModelFactory<CLIPTextModelRunner, CLIPVersion> factory;
//   factory.register_version(OPENAI_CLIP_VIT_L_14, [](backend, offload, map, prefix) {
//       return std::make_unique<CLIPTextModelRunner>(backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14);
//   });
//   auto runner = factory.create(OPENAI_CLIP_VIT_L_14, backend, false, empty_map, "");
//
// All methods are inline (header-only template).
template<typename ModelType, typename VersionEnum>
class ModelFactory {
public:
    using CreatorFn = std::function<std::unique_ptr<ModelType>(
        ggml_backend_t backend,
        bool offload_params_to_cpu,
        const String2TensorStorage& tensor_map,
        const std::string& prefix
    )>;

    // Register a creator function for a specific version.
    void register_version(VersionEnum version, CreatorFn creator) {
        registry_[version] = std::move(creator);
    }

    // Create a model instance for the given version.
    // Throws std::runtime_error("Unknown model version") if version not registered.
    std::unique_ptr<ModelType> create(
        VersionEnum version,
        ggml_backend_t backend,
        bool offload_params_to_cpu,
        const String2TensorStorage& tensor_map,
        const std::string& prefix = "") const
    {
        auto it = registry_.find(version);
        if (it == registry_.end()) {
            throw std::runtime_error("Unknown model version");
        }
        return it->second(backend, offload_params_to_cpu, tensor_map, prefix);
    }

    // Returns true if a creator for this version has been registered.
    bool has_version(VersionEnum version) const {
        return registry_.count(version) > 0;
    }

    // Returns a list of all registered versions in map order.
    std::vector<VersionEnum> registered_versions() const {
        std::vector<VersionEnum> versions;
        versions.reserve(registry_.size());
        for (auto& kv : registry_) {
            versions.push_back(kv.first);
        }
        return versions;
    }

private:
    std::map<VersionEnum, CreatorFn> registry_;
};

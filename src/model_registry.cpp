#include "model_registry.hpp"

ModelRegistry* ModelRegistry::instance() {
    static ModelRegistry instance;
    return &instance;
}

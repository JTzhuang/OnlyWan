#include "src/config_loader.hpp"
#include <iostream>

int main() {
    try {
        WanLoadConfig cfg = ConfigLoader::load_config("/data/zhongwang2/jtzhuang/projects/OnlyWan/test_config_cuda.json");
        std::cout << "✓ Config loaded successfully" << std::endl;
        std::cout << "  backend: " << cfg.backend << std::endl;
        std::cout << "  n_threads: " << cfg.n_threads << std::endl;
        std::cout << "  transformer_path: " << cfg.transformer_path << std::endl;

        if (cfg.backend == "cuda") {
            std::cout << "\n✓ SUCCESS: backend correctly read as 'cuda'" << std::endl;
            return 0;
        } else {
            std::cout << "\n✗ FAILURE: backend is '" << cfg.backend << "', expected 'cuda'" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "✗ Exception: " << e.what() << std::endl;
        return 1;
    }
}

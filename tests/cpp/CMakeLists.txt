# wan_add_test(target_name source_file)
# Creates a test executable linked against wan-cpp with proper include paths.
# Tests access model headers (clip.hpp, t5.hpp, etc.) through wan-cpp's PUBLIC
# include directories -- this is safe because all model headers are fully
# header-only with implicitly inline member functions.
function(wan_add_test target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE wan-cpp)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src          # Model headers (clip.hpp, t5.hpp, etc.)
        ${CMAKE_SOURCE_DIR}/include      # Public API headers
        ${CMAKE_CURRENT_SOURCE_DIR}      # Test headers (test_framework.hpp, etc.)
    )
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

wan_add_test(test_factory     test_factory.cpp)
wan_add_test(test_clip        test_clip.cpp)
wan_add_test(test_t5          test_t5.cpp)
wan_add_test(test_vae         test_vae.cpp)
wan_add_test(test_transformer test_transformer.cpp)

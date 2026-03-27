# Third-party dependencies for wan-cpp

# spdlog - header-only, already in thirdparty/spdlog/
if(NOT TARGET spdlog::spdlog)
    add_library(spdlog INTERFACE)
    target_include_directories(spdlog INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
    )
    add_library(spdlog::spdlog ALIAS spdlog)
endif()

# ZIP library (miniz-based)
set(Z_TARGET zip)
add_library(${Z_TARGET} OBJECT zip.c zip.h miniz.h)
target_include_directories(${Z_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# libnpy - header-only .npy file I/O
if(NOT TARGET libnpy)
    add_library(libnpy INTERFACE)
    target_include_directories(libnpy INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

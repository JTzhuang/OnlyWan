# Third-party dependencies for wan-cpp

# ZIP library (miniz-based)
set(Z_TARGET zip)
add_library(${Z_TARGET} OBJECT zip.c zip.h miniz.h)
target_include_directories(${Z_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

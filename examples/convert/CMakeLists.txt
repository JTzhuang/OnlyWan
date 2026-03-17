# Converter tool for wan-cpp: safetensors -> GGUF

add_executable(wan-convert main.cpp)

target_link_libraries(wan-convert PRIVATE wan-cpp)

target_include_directories(wan-convert PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/wan-cpp
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${GGML_INCLUDE_DIRS}
)

target_link_libraries(wan-convert PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    m
)

install(TARGETS wan-convert
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Converter tool: wan-convert")

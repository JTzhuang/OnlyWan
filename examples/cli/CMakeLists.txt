# CLI example for wan-cpp

set(CLI_SOURCES
    main.cpp
    avi_writer.c
)

# Create wan-cli executable
add_executable(wan-cli ${CLI_SOURCES})

# Link against wan-cpp library
target_link_libraries(wan-cli PRIVATE wan-cpp)

# Propagate WAN_EMBED_VOCAB to CLI so #ifndef WAN_EMBED_VOCAB guards work correctly.
# The macro is PRIVATE on wan-cpp and does not propagate automatically.
if(WAN_EMBED_VOCAB)
    target_compile_definitions(wan-cli PRIVATE WAN_EMBED_VOCAB)
endif()

# Include directories
target_include_directories(wan-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/wan-cpp
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${GGML_INCLUDE_DIRS}
)

# Link against system libraries
target_link_libraries(wan-cli PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    m
)

# Add install target
install(TARGETS wan-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Print configuration message
message(STATUS "CLI example: wan-cli")
message(STATUS "  Sources: ${CLI_SOURCES}")

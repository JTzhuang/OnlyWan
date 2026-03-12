# CLI example for wan-cpp

set(CLI_SOURCES
    main.cpp
)

set(CLI_HEADERS
    avi_writer.h
)

# Create the wan-cli executable
add_executable(wan-cli ${CLI_SOURCES})

# Link against the wan-cpp library
target_link_libraries(wan-cli PRIVATE wan-cpp)

# Include directories
target_include_directories(wan-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/examples/cli
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

cmake_minimum_required(VERSION 3.10)
project(TerrainDemo)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Add source files
set(SOURCES
    src/main.cpp
    src/Core/Window.cpp
    src/Core/Shader.cpp
    src/Core/ShaderManager.cpp
    src/Core/Camera.cpp
    src/Core/Texture.cpp
    src/Core/stb_image_impl.cpp
    src/Grid/BaseGrid.cpp
    src/Grid/TerrainGrid.cpp
    src/Grid/GridMesh.cpp
    src/Grid/TerrainGenerator.cpp
    src/ObjectLoader/ObjectLoader.cpp
)

set(ASSIMP_SRC_DIR ${CMAKE_SOURCE_DIR}/include/assimp)
set(ASSIMP_BUILD_DIR ${ASSIMP_SRC_DIR}/build)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${ASSIMP_SRC_DIR}/include      # Assimp's source include directory
    ${ASSIMP_BUILD_DIR}/include   # Assimp's build include directory (for generated headers like config.h)
    ${CMAKE_SOURCE_DIR}/src
)

# Define GL_SILENCE_DEPRECATION to silence OpenGL deprecation warnings on macOS
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GL_SILENCE_DEPRECATION
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${ASSIMP_BUILD_DIR}/lib         # Assimp built static/shared libs
)


# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    ${ASSIMP_BUILD_DIR}/bin/libassimp.dylib
)

# Copy shader files to build directory
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.glsl")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    configure_file(
        ${SHADER_FILE}
        ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}
        COPYONLY
    )
endforeach()

# Create shaders directory in build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Copy resources directory recursively to build directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying resources to build directory"
)

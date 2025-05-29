cmake_minimum_required(VERSION 3.12)
project(TerrainDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Source files ===
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

# === Include directories ===
include_directories(
    include
    external/glm-1.0.1
    ${CMAKE_SOURCE_DIR}/src
)

# === Platform-specific setup ===
if(WIN32)
    # GLFW from source
    add_subdirectory(external/glfw)
    
    # GLEW as static library
    add_library(glew STATIC
        external/glew/src/glew.c
        
    )
    target_include_directories(glew PUBLIC external/glew/include)
    
    # Define macro
    add_definitions(-DGLEW_STATIC)
    
    # OpenGL linking on Windows
    set(OPENGL_LIBS opengl32)
    
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    
    # OpenGL linking on UNIX
    set(OPENGL_LIBS OpenGL::GL dl pthread)
endif()

# === Executable ===
add_executable(${PROJECT_NAME} ${SOURCES})

# === Include dirs (more if needed) ===
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# === Define for macOS warnings ===
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GL_SILENCE_DEPRECATION
)

# === Link libraries ===
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        glfw
        glew
        ${OPENGL_LIBS}
    )
else()
    target_link_libraries(${PROJECT_NAME}
        glfw
        glew
        ${OPENGL_LIBS}
    )
endif()

# === Copy shaders ===
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.glsl")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    configure_file(
        ${SHADER_FILE}
        ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}
        COPYONLY
    )
endforeach()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# === Copy resources ===
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying resources to build directory"
)

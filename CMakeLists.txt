cmake_minimum_required(VERSION 3.16)

# ---- Project ----
project(BuildingSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Helpful defaults on MSVC
if(MSVC)
  add_compile_options(/permissive- /W3 /MP)
  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

# ---- Dependencies (via vcpkg toolchain) ----
# Configure with:
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#         -DCMAKE_TOOLCHAIN_FILE=C:/Users/yildiz/vcpkg/scripts/buildsystems/vcpkg.cmake
#         -DVCPKG_TARGET_TRIPLET=x64-windows
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW   CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# ---- Sources ----
# ---- Sources (auto-collect all project .cpp files) ----
# This replaces your manual set(SOURCES ...) block
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# If you intentionally want to exclude something, do it here, e.g.:
# list(REMOVE_ITEM PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/src/Core/AudioManager.cpp")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})


# Includes: OpenGL adds includes via variable; the others via targets
target_include_directories(${PROJECT_NAME} PRIVATE
  ${OPENGL_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

if(APPLE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()

# ---- Link ----
target_link_libraries(${PROJECT_NAME} PRIVATE
  OpenGL::GL
  glfw
  GLEW::GLEW
  assimp::assimp
)

# ---- Assets: copy NEXT TO THE EXE (handles Debug/Release) ----
# Shaders (individual files)
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.glsl")
if(SHADER_FILES)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders")
  foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME "${SHADER_FILE}" NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${SHADER_FILE}"
              "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/${SHADER_NAME}")
  endforeach()
endif()

# resources/ folder
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
    COMMENT "Copying resources next to exe")
endif()

# Objects/ folder
if(EXISTS "${CMAKE_SOURCE_DIR}/Objects")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Objects"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Objects"
    COMMENT "Copying Objects next to exe")
endif()

# Optional files referenced with relative paths in code
if(EXISTS "${CMAKE_SOURCE_DIR}/ObjectPaths.txt")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/ObjectPaths.txt"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ObjectPaths.txt")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/include/music.mp3")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/include/music.mp3"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/music.mp3")
endif()

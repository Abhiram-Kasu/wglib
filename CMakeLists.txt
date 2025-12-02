cmake_minimum_required(VERSION 3.22) # CMake version check
project(wglib)                       # Create project "wglib"
set(CMAKE_CXX_STANDARD 23)           # Enable C++20 standard

# Generate compile_commands.json for IDEs and language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp")

add_executable(wglib ${SOURCES})

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

if (EMSCRIPTEN)
    set_target_properties(wglib PROPERTIES SUFFIX ".html")
    target_link_libraries(wglib PRIVATE emdawnwebgpu_cpp webgpu_glfw)
    target_link_options(wglib PRIVATE "-sASYNCIFY=1" "-sUSE_GLFW=3")
else ()
    target_link_libraries(wglib PRIVATE webgpu_dawn webgpu_glfw glfw)
endif ()

# Add src directory to include path
target_include_directories(wglib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Fetch and integrate GLM library
include(FetchContent)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863 # refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(wglib PRIVATE glm::glm)
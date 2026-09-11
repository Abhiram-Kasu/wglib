cmake_minimum_required(VERSION 3.22)

project(wglib
    VERSION 0.1.0
    DESCRIPTION "A C++ WebGPU library for 2D graphics and compute applications"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

option(WGLIB_BUILD_EXAMPLES "Build the wglib example applications" ON)
option(WGLIB_BUILD_TESTS "Build wglib tests" OFF)
if(EMSCRIPTEN)
    set(WGLIB_INSTALL_DEFAULT OFF)
else()
    set(WGLIB_INSTALL_DEFAULT ON)
endif()
option(WGLIB_INSTALL "Enable installation and CMake package export" ${WGLIB_INSTALL_DEFAULT})
option(WGLIB_FETCH_DEPENDENCIES "Fetch GLM and Dawn dependencies" ON)
set(WGLIB_LIBRARY_TYPE "SHARED" CACHE STRING "Library type for native builds (STATIC or SHARED)")
set_property(CACHE WGLIB_LIBRARY_TYPE PROPERTY STRINGS STATIC SHARED)

set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ standard used to build wglib and its dependencies")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require the selected C++ standard")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dawn performs a few configure-time compiler checks with
# check_cxx_source_compiles(), which do not inherit target compile features.
# Emscripten reports an old default language mode for those checks even though
# its compiler supports modern C++, so provide an explicit baseline flag. The
# wglib targets still request C++23 through target_compile_features().
if(EMSCRIPTEN AND NOT CMAKE_CXX_FLAGS MATCHES "(^| )-std=")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20" CACHE STRING
        "Flags passed to the C++ compiler" FORCE)
endif()

if(WGLIB_FETCH_DEPENDENCIES)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863
    )
    FetchContent_MakeAvailable(glm)
endif()

# Dawn is kept as a pinned submodule so native and WebAssembly builds use the
# same WebGPU implementation.
set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
set(DAWN_ENABLE_PIC ON CACHE BOOL "" FORCE)
set(DAWN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_CMD_TOOLS OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_IR_BINARY OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_GLSL_VALIDATOR OFF CACHE BOOL "" FORCE)

if(EMSCRIPTEN)
    set(DAWN_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_D3D11 OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_D3D12 OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_METAL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_NULL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_DESKTOP_GL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_OPENGLES OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_VULKAN OFF CACHE BOOL "" FORCE)
    set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(DAWN_BUILD_NODE_BINDINGS OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(dawn)

set(WGLIB_CORE_SOURCES
    src/wglib/CoreEngine.cpp
    src/wglib/CoreInput.cpp
    src/wglib/CoreRenderer.cpp
    src/wglib/WindowManager.cpp
    src/wglib/render_layer/CircleRenderLayer.cpp
    src/wglib/render_layer/RectangleRenderLayer.cpp
    src/wglib/render_layer/RenderLayer.cpp
    src/wglib/render_layer/TextureRenderLayer.cpp
    src/wglib/render_layer/TriangleRenderLayer.cpp
    src/wglib/compute/ComputeEngine.cpp
)

if(EMSCRIPTEN)
    set(WGLIB_EFFECTIVE_LIBRARY_TYPE STATIC)
else()
    set(WGLIB_EFFECTIVE_LIBRARY_TYPE ${WGLIB_LIBRARY_TYPE})
endif()

add_library(wglib ${WGLIB_EFFECTIVE_LIBRARY_TYPE} ${WGLIB_CORE_SOURCES})
add_library(wglib::wglib ALIAS wglib)

if(WIN32 AND WGLIB_EFFECTIVE_LIBRARY_TYPE STREQUAL "SHARED")
    set_target_properties(wglib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_compile_features(wglib PUBLIC cxx_std_23)
target_include_directories(wglib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<$<BOOL:${EMSCRIPTEN}>:$<BUILD_INTERFACE:${DAWN_BUILD_GEN_DIR}/src/emdawnwebgpu/include>>
        $<BUILD_INTERFACE:${DAWN_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${DAWN_BUILD_GEN_DIR}/include>
        $<BUILD_INTERFACE:${DAWN_GLFW_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wglib
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wglib/compute
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wglib/compute/ExampleLayers
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wglib/render_layer
)

if(EMSCRIPTEN)
    target_link_libraries(wglib PRIVATE emdawnwebgpu_cpp)
else()
    target_link_libraries(wglib PRIVATE dawn::webgpu_dawn dawn::dawn_glfw glfw)
endif()

if(DEFINED glm_SOURCE_DIR)
    target_include_directories(wglib SYSTEM PUBLIC
        $<BUILD_INTERFACE:${glm_SOURCE_DIR}>
    )
elseif(TARGET glm::glm)
    get_target_property(WGLIB_GLM_INCLUDE_DIRS glm::glm INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(wglib SYSTEM PUBLIC ${WGLIB_GLM_INCLUDE_DIRS})
endif()

# Example compute layers are kept out of the core engine while remaining
# available as a separately linkable target for the bundled demonstrations.
add_library(wglib_examples STATIC
    src/wglib/compute/ExampleLayers/ConwaysGameOfLife.cpp
    src/wglib/compute/ExampleLayers/ParticleSimulation.cpp
)
add_library(wglib::examples ALIAS wglib_examples)
set_target_properties(wglib_examples PROPERTIES EXPORT_NAME examples)
target_compile_features(wglib_examples PUBLIC cxx_std_23)
target_include_directories(wglib_examples
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wglib/compute/ExampleLayers
)
target_link_libraries(wglib_examples PUBLIC wglib::wglib)

if(WGLIB_BUILD_EXAMPLES)
    add_executable(wglib_demo examples/desktop/main.cpp)
    target_link_libraries(wglib_demo PRIVATE wglib::wglib wglib::examples)

    if(EMSCRIPTEN)
        set_target_properties(wglib_demo PROPERTIES SUFFIX ".html")
        target_link_options(wglib_demo PRIVATE
            "-sEXPORTED_FUNCTIONS=_main,_get_argc,_glfwSetWindowSize,_get_window_height,_get_window_width"
            "-sEXPORTED_RUNTIME_METHODS=callMain"
            "-sNO_EXIT_RUNTIME=1"
            "-sASYNCIFY=1"
            "-sUSE_GLFW=3"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sTEXTDECODER=2"
            "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/shaders@/src/shaders"
            "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
        )
    endif()

    add_custom_command(TARGET wglib_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:wglib_demo>/shaders
        COMMENT "Copying wglib shaders next to the example"
    )
endif()

if(WGLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(WGLIB_INSTALL)
    install(TARGETS wglib wglib_examples
        EXPORT wglibTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(DIRECTORY shaders/ DESTINATION ${CMAKE_INSTALL_DATADIR}/wglib/shaders)

    if(TARGET glfw)
        install(DIRECTORY ${DAWN_GLFW_DIR}/include/GLFW
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    if(WGLIB_FETCH_DEPENDENCIES AND DEFINED glm_SOURCE_DIR)
        install(DIRECTORY ${glm_SOURCE_DIR}/glm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    configure_package_config_file(
        cmake/wglibConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/wglibConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wglib
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/wglibConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
    )

    install(EXPORT wglibTargets
        FILE wglibTargets.cmake
        NAMESPACE wglib::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wglib
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/wglibConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/wglibConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wglib
    )
endif()

set(CPACK_PACKAGE_NAME "wglib")
set(CPACK_PACKAGE_VENDOR "Abhiram Kasu")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A C++ WebGPU library for 2D graphics and compute applications")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "wglib-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)

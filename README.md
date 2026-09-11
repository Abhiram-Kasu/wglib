# wglib

wglib is a C++23 library for building 2D graphics and GPU-compute applications with WebGPU. The same public API can target a native desktop window or WebAssembly in a browser.

The project is distributed as a reusable CMake library. Native builds produce a shared library by default; static native builds and WebAssembly builds are also supported.

## What you get

- A small engine with a callback-based update loop.
- WebGPU rendering through Dawn on desktop and Emscripten on the web.
- Built-in rectangle, circle, triangle, and texture render layers.
- Typed, asynchronous compute-layer callbacks.
- WGSL shader support, including bundled example shaders.
- Logical coordinates with aspect-ratio-preserving resizing.
- Mouse position and button state through `InputManager`.
- Installable CMake targets and TGZ/ZIP packages.

## Requirements

### Native desktop

- CMake 3.22 or newer.
- A compiler and standard library with C++23 support. Clang is the primary tested toolchain; GCC 13+ and MSVC 2022+ are expected to work.
- Ninja is recommended, although another CMake generator can be used.
- A platform WebGPU-capable graphics environment and the normal GLFW system prerequisites.

GLM is fetched by CMake. Dawn is kept as a pinned submodule and fetches the Dawn dependencies needed by the selected build.

### WebAssembly

- The Emscripten SDK (`emcmake`, `em++`, and `emrun`).
- Ninja.
- A local HTTP server for testing the generated page.

The web build uses Emscripten's WebGPU implementation and does not install the native Dawn package.

## Get the source

Initialize the direct Dawn submodule after cloning:

```bash
git clone https://github.com/Abhiram-Kasu/wglib.git
cd wglib
git submodule update --init dawn
```

Only the direct `dawn` submodule is required in the checkout. Dawn's CMake dependency step obtains the dependencies it needs. A recursive submodule checkout is not necessary.

## Quick start: native desktop

Configure and build the bundled demo:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DWGLIB_BUILD_EXAMPLES=ON

cmake --build build --target wglib_demo --parallel
./build/wglib_demo
```

The demo runs Conway's Game of Life by default. It also contains several small demonstrations:

```text
./build/wglib_demo 0    particle simulation
./build/wglib_demo 1    simple refactor/render test
./build/wglib_demo 2    Conway's Game of Life
./build/wglib_demo 3    triangle rendering
./build/wglib_demo 4    mouse interaction test
```

## Quick start: WebAssembly

Activate an Emscripten SDK, then configure and build with the Emscripten toolchain:

```bash
emcmake cmake -S . -B build_web -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DWGLIB_BUILD_EXAMPLES=ON \
  -DWGLIB_INSTALL=OFF \
  -DWGLIB_BUILD_TESTS=OFF

cmake --build build_web --target wglib_demo --parallel
```

This generates `build_web/wglib_demo.html` and its companion files. Serve the build directory rather than opening the HTML file directly:

```bash
cd build_web
python3 -m http.server 8000
```

Open <http://localhost:8000/wglib_demo.html> in a WebGPU-capable browser.

## Use wglib from another CMake project

There are two supported integration styles. Installing a package is the best choice for applications that consume a released build. Adding the source tree is useful when developing wglib and the application together.

### Option 1: install and `find_package`

Build and install the native library:

```bash
cmake -S . -B build_install -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DWGLIB_BUILD_EXAMPLES=OFF \
  -DWGLIB_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"

cmake --build build_install --target wglib --parallel
cmake --install build_install
```

The install contains public headers, the library, CMake package files, GLM, the required WebGPU/GLFW headers, and the runtime WGSL shaders.

In the consuming project:

```cmake
cmake_minimum_required(VERSION 3.22)
project(my_webgpu_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wglib CONFIG REQUIRED)

add_executable(my_webgpu_app main.cpp)
target_link_libraries(my_webgpu_app PRIVATE wglib::wglib)
```

Configure that project with the install prefix:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="/path/to/wglib/install"
cmake --build build --parallel
```

Use `wglib::examples` in addition to `wglib::wglib` when using the compiled example compute layers such as Conway's Game of Life or the particle simulation:

```cmake
target_link_libraries(my_webgpu_app PRIVATE wglib::wglib wglib::examples)
```

The `ExampleLayer<N>` template is header-only and does not require `wglib::examples`.

### Option 2: add a checked-out source tree

When the wglib checkout has its `dawn` submodule initialized, a consuming project can add it directly:

```cmake
set(WGLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WGLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WGLIB_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/wglib wglib-build)

add_executable(my_webgpu_app main.cpp)
target_link_libraries(my_webgpu_app PRIVATE wglib::wglib)
```

For a Git-based `FetchContent` integration, fetch the direct Dawn submodule as well:

```cmake
include(FetchContent)

FetchContent_Declare(wglib
  GIT_REPOSITORY https://github.com/Abhiram-Kasu/wglib.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  GIT_SUBMODULES dawn
  GIT_SUBMODULES_RECURSE FALSE
)

set(WGLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WGLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WGLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(wglib)

add_executable(my_webgpu_app main.cpp)
target_link_libraries(my_webgpu_app PRIVATE wglib::wglib)
```

For reproducible builds, replace `main` with a published release tag such as `v0.1.0` when one is available.

## CMake options and targets

The most useful configuration options are:

| Option | Default | Purpose |
|---|---:|---|
| `WGLIB_BUILD_EXAMPLES` | `ON` | Build `wglib_demo`. |
| `WGLIB_BUILD_TESTS` | `OFF` | Build the API smoke test and enable CTest. |
| `WGLIB_INSTALL` | `ON` native, `OFF` web | Generate install rules and export the CMake package. |
| `WGLIB_FETCH_DEPENDENCIES` | `ON` | Fetch GLM and allow dependency setup during configuration. |
| `WGLIB_LIBRARY_TYPE` | `SHARED` native | Select `SHARED` or `STATIC` for native `wglib`. WebAssembly always uses a static library. |

The main targets are:

| Target | Description |
|---|---|
| `wglib::wglib` | Core rendering, window, input, and compute engine. |
| `wglib::examples` | Compiled built-in compute examples. Links publicly to `wglib::wglib`. |
| `wglib_demo` | Bundled desktop/WebAssembly demo when examples are enabled. |
| `wglib_api_smoke` | Small link/API test when tests are enabled. |

Useful configurations:

```bash
# Debug library plus tests
cmake -S . -B build_debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DWGLIB_BUILD_TESTS=ON

# Native static library
cmake -S . -B build_static -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DWGLIB_LIBRARY_TYPE=STATIC \
  -DWGLIB_BUILD_EXAMPLES=OFF
```

CMake always enables `CMAKE_EXPORT_COMPILE_COMMANDS`. The generated database lives inside the selected build directory. A root-level `compile_commands.json` symlink is optional and is ignored by Git.

## First application

The engine owns the WebGPU device, window, renderer, and frame loop. Create render layers once, draw them from `OnUpdate`, and call `Start()`:

```cpp
#include <wglib/CoreEngine.hpp>
#include <wglib/render_layer/CircleRenderLayer.hpp>
#include <wglib/render_layer/RectangleRenderLayer.hpp>

int main() {
  wglib::Engine engine({800, 600}, "My application");

  auto rectangle =
      engine.CreateRenderLayer<wglib::render_layers::RectangleRenderLayer>(
          glm::vec2{100, 100},
          glm::vec2{200, 150},
          glm::vec3{1, 0, 0});

  auto circle =
      engine.CreateRenderLayer<wglib::render_layers::CircleRenderLayer>(
          glm::vec2{400, 300},
          75.0f,
          glm::vec3{0, 0, 1});

  engine.OnUpdate([&](float delta_time) {
    (void)delta_time;
    engine.Draw(rectangle);
    engine.Draw(circle);
  });

  engine.Start();
}
```

`CreateRenderLayer` returns a typed handle. Keep that handle alive while the layer is in use. Positions and sizes use the engine's logical coordinate system; resizing preserves the logical aspect ratio.

Useful engine methods include:

- `OnUpdate(callback)`: register the per-frame callback. The callback receives delta time in seconds.
- `Draw(handle)`: queue a render layer for the current frame.
- `SetTargetFPS(fps)`: set a target frame interval; pass a non-positive value to disable the limit.
- `Input()`: access mouse position and button state.
- `GetWindow()`, `GetDevice()`, and `GetWindowSize()`: access the underlying window/device or logical size.

## Render layers

Include the layer you need from `wglib/render_layer/`:

- `RectangleRenderLayer(position, size, color)` with `setPosition`, `setSize`, and `setColor`.
- `CircleRenderLayer(origin, radius, color, resolution = 50)` with setters for origin, radius, resolution, and color.
- `TriangleRenderLayer(std::array<Vertex, 3>)` for custom colored triangles.
- `TextureRenderLayer(width, height)` or `TextureRenderLayer(texture, width, height)` for displaying a `wgpu::Texture`.

To create a custom layer, derive from `wglib::render_layers::RenderLayer` and implement:

```cpp
Render(wgpu::RenderPassEncoder&)
InitRes(const wgpu::Device&, wgpu::TextureFormat,
        const wgpu::BindGroupLayout&)
UpdateRes(const wgpu::Device&)
```

`Render` encodes draw calls, `InitRes` creates GPU resources, and `UpdateRes` uploads mutable per-frame data.

## Compute layers

Compute layers are typed around their callback result. Initialize a layer once, then queue it whenever work should run:

```cpp
auto compute =
    engine.InitComputeLayer<wglib::compute::ExampleLayer<1024>>(2.0f);

engine.PushComputeLayer(
    compute,
    [](std::optional<std::span<const float, 1024>> result) {
      if (!result) {
        return; // GPU readback is not ready yet.
      }
      // Consume the typed result.
    });
```

`InitComputeLayer` constructs the layer and initializes its GPU resources. Keep the returned handle alive. `PushComputeLayer` queues one execution and invokes the callback after the submitted GPU work completes.

The built-in compute layers are:

- `ExampleLayer<N>`: multiplies a generated array by a scalar and returns an optional fixed-size span.
- `ConwaysGameOfLifeComputeLayer`: runs a ping-pong cellular automaton and returns a texture.
- `ParticleSimulationLayer`: simulates particles and returns an optional texture.

For a continuously running simulation, queue the next operation from the completion callback:

```cpp
std::function<void()> run_next;
run_next = [&] {
  engine.PushComputeLayer(compute, [&](wgpu::Texture texture) {
    texture_layer->setTexture(std::move(texture));
    run_next();
  });
};

run_next();
```

When a compute shader writes a texture, display it with `TextureRenderLayer`:

```cpp
auto texture_layer =
    engine.CreateRenderLayer<wglib::render_layers::TextureRenderLayer>(
        1280, 720);

engine.OnUpdate([&](float) { engine.Draw(texture_layer); });
```

### Writing a custom compute layer

Derive from `ComputeLayer<T>`, where `T` is the result type delivered to the callback:

```cpp
class DoubleValues
    : public wglib::compute::ComputeLayer<std::vector<float>> {
protected:
  void InitImpl(wgpu::Device& device) override {
    // Create buffers, bind groups, pipelines, and other resources.
  }

  void ComputeImpl(wgpu::CommandEncoder& encoder,
                   wgpu::Queue& queue,
                   wglib::Engine& engine) override {
    // Upload inputs, encode the WGSL compute pass, submit it, and
    // asynchronously prepare the result.
  }

  std::vector<float> getResultImpl() override {
    return result_;
  }

private:
  std::vector<float> result_;
};
```

The three protected methods have distinct responsibilities:

1. `InitImpl` runs once and allocates GPU resources.
2. `ComputeImpl` runs for each queued execution and submits GPU work.
3. `getResultImpl` returns the value passed to the completion callback.

WGSL files can be loaded with `wglib::util::shaderPath("my_shader.wgsl")` and `wglib::util::createShaderModuleFromFile(...)`. Native applications normally run with a `shaders/` directory beside the executable or from the installed layout. The WebAssembly build packages shaders into the Emscripten virtual filesystem under `/src/shaders/`.

## Frame and compute order

Each frame follows this order:

1. Poll window events.
2. Process completed WebGPU callbacks from earlier submissions.
3. Run the user's `OnUpdate` callback.
4. Submit compute work queued during the update callback.
5. Render layers queued with `Draw`.

As a result, a compute callback is asynchronous and may run on a later frame. For an every-frame simulation, re-queue from the completion callback or use an in-flight flag so work does not pile up.

## Tests and validation

Configure tests and run them with CTest:

```bash
cmake -S . -B build_tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DWGLIB_BUILD_EXAMPLES=OFF \
  -DWGLIB_BUILD_TESTS=ON

cmake --build build_tests --target wglib wglib_examples wglib_api_smoke --parallel
ctest --test-dir build_tests --output-on-failure
```

The repository also contains an installed-package consumer at `tests/consumer`:

```bash
cmake --install build_install
cmake -S tests/consumer -B build_consumer -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build_consumer --parallel
./build_consumer/wglib_consumer
```

## Packaging

After installing/configuring a native build, create distributable TGZ and ZIP archives:

```bash
cmake -S . -B build_package -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DWGLIB_BUILD_EXAMPLES=OFF \
  -DWGLIB_BUILD_TESTS=ON
cmake --build build_package --target wglib wglib_examples wglib_api_smoke --parallel
ctest --test-dir build_package --output-on-failure
cmake --install build_package
cpack --config build_package/CPackConfig.cmake
```

Packages are written to `build_package/packages/` and contain the versioned wglib library, public headers, CMake package metadata, bundled dependencies, and shaders.

## GitHub Actions

Two workflows are checked in under `.github/workflows/`:

- **Build and Deploy Web (Emscripten)**: builds the demo with Emscripten and publishes the generated page to GitHub Pages on pushes to `main` or a manual dispatch.
- **Build and Publish Library**: on a `v*` tag, builds native packages on Ubuntu, macOS, and Windows, runs the smoke test, uploads the package artifacts, and creates a GitHub Release. It can also be run manually for package artifacts.

The Pages workflow initializes only the direct Dawn submodule. This keeps CI checkout reliable while Dawn's CMake dependency setup fetches the dependencies required by the web build.

## Project layout

```text
include/wglib/                  public headers installed for consumers
src/wglib/                      compiled library implementation
examples/desktop/main.cpp       bundled demo entry point
shaders/                        WGSL shaders copied/packaged with the build
dawn/                           pinned Dawn WebGPU submodule
cmake/wglibConfig.cmake.in      installed-package configuration template
tests/                           API smoke test and installed consumer
.github/workflows/              Pages and release workflows
CMakeLists.txt                  library, examples, tests, install, and CPack
```

Build directories such as `build/`, `build_debug/`, `build_web/`, and `cmake-build-*` are intentionally ignored. Keep build output out of source control.

## Troubleshooting

### `dawn` is missing during configuration

Initialize the direct submodule:

```bash
git submodule update --init dawn
```

### The demo cannot find a shader

Run the executable from a directory where the expected `shaders/` directory is available, or use the installed layout produced by `cmake --install`. The CMake demo copies the shader directory next to its executable.

### Emscripten configuration fails

Confirm that the SDK is activated and that both `emcmake` and Ninja are available:

```bash
emcc --version
emcmake --version
ninja --version
```

### CMake generated files appear in `git status`

Use an out-of-source build directory. The repository ignores the standard `build*` and `cmake-build-*` patterns, as well as `compile_commands.json`.

## Roadmap

- Keyboard event processing.
- A manual engine-tick mode for applications that need to control frame advancement.

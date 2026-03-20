# wglib

A modern C++23 library for creating 2D graphics and compute applications using WebGPU. Build native desktop applications or compile to WebAssembly for the web.

## Features

- **Cross-platform rendering**: Write once, run on desktop (via Dawn) or web (via Emscripten)
- **WebGPU-based**: Modern GPU API with compute shader support
- **2D rendering primitives**: Built-in support for rectangles, circles, and textures
- **Compute layers**: Run GPU compute shaders for parallel processing with typed, callback-based results
- **Update loop**: Simple callback-based update cycle for animations and game logic
- **Modern C++23**: Leverages latest C++ features for clean, expressive code

## Requirements

### Desktop Build
- CMake 3.22 or higher
- C++23 compatible compiler (GCC 13+, Clang 16+, or MSVC 2022+). Only tested with Clang however.
- GLFW (fetched automatically)
- Dawn WebGPU implementation (included as submodule)
- GLM library (fetched automatically)

### Web Build
- Emscripten SDK
- Ninja build system
- All other dependencies are handled by Emscripten

## Building

### Getting the Code

```bash
# Clone the repository with submodules
git clone https://github.com/Abhiram-Kasu/wglib.git
cd wglib
git submodule update --init
```

### Desktop Build

```bash
# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run the example
./wglib
```

### Web Build

```bash
mkdir build_web && cd build_web
emcmake cmake ..
cmake --build .

# Serve the result (use any web server)
python3 -m http.server 8000
# Open http://localhost:8000/wglib.html in your browser
```

## How to Use

### Basic Setup

```cpp
#include "lib/CoreEngine.hpp"
#include "lib/render_layer/CircleRenderLayer.hpp"
#include "lib/render_layer/RectangleRenderLayer.hpp"

int main() {
  // Create engine with window size and title
  wglib::Engine engine({800, 600}, "My Application");

  // Create render layers
  wglib::render_layers::RectangleRenderLayer rect(
      glm::vec2{100, 100},         // position (x, y)
      glm::vec2{200, 150},         // size (width, height)
      glm::vec3{1.0f, 0.0f, 0.0f} // color (red)
  );

  wglib::render_layers::CircleRenderLayer circle(
      glm::vec2{400, 300},         // origin (x, y)
      75.0f,                        // radius
      glm::vec3{0.0f, 0.0f, 1.0f} // color (blue)
  );

  // Set up update callback — called once per frame with delta time in seconds
  engine.OnUpdate([&](float deltaTime) {
    engine.Draw(rect);
    engine.Draw(circle);
  });

  // Start the main loop
  engine.Start();
}
```

---

## Compute Layers

Compute layers are the primary mechanism for running GPU compute shaders in wglib. Each compute layer encapsulates a complete GPU pipeline — buffers, bind groups, and compute pass encoding — and delivers results back to the CPU through a typed callback.

### How Compute Layers Work

The lifecycle of a compute layer has three phases:

1. **Initialization** (`InitImpl`) — Create all GPU resources once: buffers, the compute pipeline, and bind groups. Called automatically when you call `engine.InitComputeLayer`.
2. **Execution** (`ComputeImpl`) — Upload data, encode the compute pass, and submit GPU commands. Called by the engine each time the layer is in the compute queue.
3. **Result retrieval** (`getResultImpl`) — Return the result after the GPU has finished. Called internally and forwarded to the user callback registered with `PushComputeLayer`.

### The `ComputeLayer<T>` Template

Every compute layer inherits from `wglib::compute::ComputeLayer<T>`, where `T` is the type returned to the callback. This provides full type safety — no casting required.

```cpp
template <typename T>
class ComputeLayer : public IComputeLayer {
public:
  using ResultType = T;  // Exposed so the engine can deduce the result type

protected:
  // Called once during InitComputeLayer — allocate GPU resources here
  virtual auto InitImpl(wgpu::Device &device) -> void = 0;

  // Called each frame when the layer is in the compute queue
  // Encode your compute pass and submit GPU commands here
  virtual auto ComputeImpl(wgpu::CommandEncoder &encoder, wgpu::Queue &queue) -> void = 0;

  // Return the result after the GPU has finished
  // The return type must match T
  virtual auto getResultImpl() -> T = 0;
};
```

### Engine API for Compute

```cpp
// 1. Create and initialize a compute layer (constructs LayerType with args...)
//    Returns a typed handle — keep it alive for as long as you need the layer
auto handle = engine.InitComputeLayer<LayerType>(args...);

// 2. Queue the layer for execution this frame with a typed callback
//    The callback receives a value of type LayerType::ResultType
engine.PushComputeLayer(handle, [](ResultType result) {
  // Use result here
});

// 3. Call engine.Start() to begin the main loop
engine.Start();
```

`InitComputeLayer` constructs the layer with the provided arguments, calls `InitImpl` on the GPU device, and returns a `ComputeLayerHandle<T>`. The handle must remain in scope for the lifetime of the layer.

`PushComputeLayer` enqueues the layer for the **current frame**. Compute layers are processed at the start of each frame before rendering. To run a layer every frame, call `PushComputeLayer` again from within the callback (see the Conway's Game of Life example below).

---

### Example 1: Array Multiplication (CPU Readback)

This example uses the built-in `ExampleLayer<N>`, which multiplies an array of N floats by a scalar on the GPU and reads the results back to the CPU.

**Result type**: `std::optional<std::span<const float, N>>`

- Returns `std::nullopt` if the GPU mapping has not completed yet.
- Returns a `std::span` over the mapped staging buffer once the data is ready.

```cpp
#include "lib/CoreEngine.hpp"
#include "lib/compute/ExampleLayers/ExampleLayer.hpp"
#include "lib/render_layer/RectangleRenderLayer.hpp"
#include "lib/render_layer/CircleRenderLayer.hpp"
#include <numbers>
#include <ranges>

int main() {
  wglib::Engine engine({500, 500}, "Compute Example");

  wglib::render_layers::RectangleRenderLayer rect(
      glm::vec2{0, 0}, glm::vec2{100, 100}, glm::vec3{0.0f, 1.0f, 0.0f});
  wglib::render_layers::CircleRenderLayer circle(
      glm::vec2{250, 250}, 50.0f, glm::vec3{0.0f, 0.0f, 1.0f});

  // Initialize ExampleLayer<50000>: generates [0..49999] and multiplies by π
  auto compute = engine.InitComputeLayer<wglib::compute::ExampleLayer<50000>>(
      static_cast<float>(std::numbers::pi));

  // Queue compute once — the callback fires when the GPU finishes
  engine.PushComputeLayer(
      compute, [](std::optional<std::span<const float, 50000>> res) {
        if (!res) {
          wglib::util::log("Results not ready yet");
          return;
        }
        // Print the first 10 results: 0, π, 2π, 3π, ...
        for (const auto &item : *res | std::ranges::views::take(10)) {
          wglib::util::log("Item: {}", item);
        }
      });

  engine.OnUpdate([&](float dt) {
    engine.Draw(rect);
    engine.Draw(circle);
  });

  engine.Start();
}
```

**What happens under the hood**:
1. `InitComputeLayer` constructs `ExampleLayer<50000>(π)` and calls `InitImpl`, which creates an input buffer, output buffer, staging buffer, and uniform buffer, then builds the compute pipeline and bind group.
2. `PushComputeLayer` enqueues the layer. At the start of the next frame, `ComputeImpl` uploads the input data and multiplier, encodes a compute pass (`DispatchWorkgroups(ceil(50000/64))`), copies the result into a staging buffer, and maps it asynchronously.
3. When the GPU signals completion, the engine calls your callback with the mapped span.

---

### Example 2: Conway's Game of Life (Texture Output)

This example runs a cellular automaton on the GPU and outputs the result as a `wgpu::Texture` rendered to the screen via a `TextureRenderLayer`. The computation is chained so that each callback immediately re-queues the layer for the next frame.

**Result type**: `const wgpu::Texture &`

```cpp
#include "lib/CoreEngine.hpp"
#include "lib/compute/ExampleLayers/ConwaysGameOfLife.hpp"
#include "lib/render_layer/TextureRenderLayer.hpp"

int main() {
  wglib::Engine engine({2560, 1440}, "Conway's Game of Life");

  // Initialize the compute layer with the grid dimensions
  auto compute =
      engine.InitComputeLayer<wglib::compute::ConwaysGameOfLifeComputeLayer>(
          glm::vec2{2560, 1440});

  wglib::render_layers::TextureRenderLayer textureLayer{2560, 1440};

  engine.SetTargetFPS(120.0);

  // Use a recursive lambda to re-queue the layer every frame
  std::function<void()> runIteration;
  runIteration = [&]() {
    engine.PushComputeLayer(compute, [&](wgpu::Texture texture) {
      // Update the texture layer with the new simulation state
      textureLayer.setTexture(std::move(texture));
      // Queue the next iteration
      runIteration();
    });
  };

  engine.OnUpdate([&](float) { engine.Draw(textureLayer); });

  runIteration(); // Kick off the first iteration
  engine.Start();
}
```

**Key design points**:
- `ConwaysGameOfLifeComputeLayer` uses **two ping-pong storage buffers**: each frame, the current generation is read from one buffer and the next generation is written to the other. The buffers are then swapped.
- The result texture is written directly by the compute shader and returned by reference from `getResultImpl`.
- Re-queuing inside the callback ensures the simulation runs continuously, one step per frame.

---

### Example 3: Particle Simulation (Optional Texture Output)

This example simulates thousands of particles with gravity and repulsion forces, rendering the result to a texture. The result is wrapped in `std::optional` because the texture may not be ready on the first callback.

**Result type**: `std::optional<wgpu::Texture>`

```cpp
#include "lib/CoreEngine.hpp"
#include "lib/compute/ExampleLayers/ParticleSimulation.hpp"
#include "lib/render_layer/TextureRenderLayer.hpp"

int main() {
  wglib::Engine engine({2560, 1440}, "Particle Simulation");

  // Arguments: numParticles, screenSize, particleRadius, color,
  //            initialCenter, spread, deltaTime, speed,
  //            damping, repulsionStrength, attractionRadius
  auto compute =
      engine.InitComputeLayer<wglib::compute::ParticleSimulationLayer>(
          10000, glm::vec2{2560, 1440}, 2, glm::vec4{0, 1, 1, 1},
          glm::vec2{500, 500}, 100, 0.016, 500, 0.98, 2000, 50);

  wglib::render_layers::TextureRenderLayer textureLayer{2560, 1440};

  engine.SetTargetFPS(120.0);

  std::function<void()> runIteration;
  runIteration = [&]() {
    engine.PushComputeLayer(compute, [&](std::optional<wgpu::Texture> res) {
      if (!res) {
        wglib::util::log("Texture not ready");
        return;
      }
      textureLayer.setTexture(std::move(*res));
      runIteration();
    });
  };

  engine.OnUpdate([&](float) { engine.Draw(textureLayer); });

  runIteration();
  engine.Start();
}
```

---

### Writing a Custom Compute Layer

To implement your own compute layer, subclass `ComputeLayer<T>` and implement the three protected methods.

#### Step 1 — Define the class

```cpp
#include "lib/compute/ComputeLayer.hpp"
#include "lib/CoreUtil.hpp"

// T is the type your callback will receive.
// Use std::optional<T> if the result may not be available immediately.
class MyComputeLayer : public wglib::compute::ComputeLayer<std::vector<float>> {
  wgpu::Buffer m_inputBuffer;
  wgpu::Buffer m_outputBuffer;
  wgpu::Buffer m_stagingBuffer;
  wgpu::ComputePipeline m_pipeline;
  wgpu::BindGroup m_bindGroup;

  std::vector<float> m_inputData;
  std::vector<float> m_result;

protected:
  auto InitImpl(wgpu::Device &device) -> void override {
    // Create buffers, load the shader, build pipeline and bind group
    m_inputBuffer = wglib::util::createBuffer<float,
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst>(
        device, m_inputData.size());
    m_outputBuffer = wglib::util::createBuffer<float,
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc>(
        device, m_inputData.size());
    m_stagingBuffer = wglib::util::createBuffer<float,
        wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst>(
        device, m_inputData.size());

    auto shaderModule = wglib::util::createShaderModuleFromFile(
        "../src/shaders/my_shader.wgsl", device);
    wgpu::ComputePipelineDescriptor desc{.compute = {.module = shaderModule}};
    m_pipeline = device.CreateComputePipeline(&desc);

    wgpu::BindGroupEntry entries[] = {
        {.binding = 0, .buffer = m_inputBuffer},
        {.binding = 1, .buffer = m_outputBuffer},
    };
    wgpu::BindGroupDescriptor bgDesc{
        .layout = m_pipeline.GetBindGroupLayout(0),
        .entryCount = 2,
        .entries = entries,
    };
    m_bindGroup = device.CreateBindGroup(&bgDesc);
  }

  auto ComputeImpl(wgpu::CommandEncoder &encoder, wgpu::Queue &queue)
      -> void override {
    // Upload input data
    queue.WriteBuffer(m_inputBuffer, 0, m_inputData.data(),
                      m_inputData.size() * sizeof(float));

    // Encode the compute pass
    auto pass = encoder.BeginComputePass();
    pass.SetPipeline(m_pipeline);
    pass.SetBindGroup(0, m_bindGroup);
    pass.DispatchWorkgroups(
        wglib::util::divCeil(m_inputData.size(), 64uz));
    pass.End();

    // Copy result to staging buffer for CPU readback
    encoder.CopyBufferToBuffer(m_outputBuffer, 0, m_stagingBuffer, 0,
                               m_inputData.size() * sizeof(float));

    auto commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Asynchronously map the staging buffer
    m_stagingBuffer.MapAsync(
        wgpu::MapMode::Read, 0, m_stagingBuffer.GetSize(),
        wgpu::CallbackMode::AllowSpontaneous,
        [&](wgpu::MapAsyncStatus status, wgpu::StringView) {
          if (status == wgpu::MapAsyncStatus::Success) {
            auto *ptr = static_cast<const float *>(
                m_stagingBuffer.GetConstMappedRange());
            m_result.assign(ptr, ptr + m_inputData.size());
          }
        });
  }

  auto getResultImpl() -> std::vector<float> override {
    return m_result;
  }

public:
  explicit MyComputeLayer(std::vector<float> input)
      : m_inputData(std::move(input)) {}
};
```

#### Step 2 — Write the WGSL compute shader

```wgsl
// src/shaders/my_shader.wgsl

@group(0) @binding(0) var<storage, read>       input  : array<f32>;
@group(0) @binding(1) var<storage, read_write> output : array<f32>;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let i = global_id.x;
    if (i < arrayLength(&input)) {
        output[i] = input[i] * 2.0;
    }
}
```

#### Step 3 — Use it from the engine

```cpp
int main() {
  wglib::Engine engine({800, 600}, "My Compute App");

  std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};

  auto handle = engine.InitComputeLayer<MyComputeLayer>(data);

  engine.PushComputeLayer(handle, [](std::vector<float> result) {
    for (float v : result) {
      wglib::util::log("Result: {}", v); // 2, 4, 6, 8
    }
  });

  engine.OnUpdate([](float) {});
  engine.Start();
}
```

---

### Continuous vs. One-Shot Compute

| Pattern | When to use | How |
|---|---|---|
| **One-shot** | Compute once and read back results (e.g., data preprocessing) | Call `PushComputeLayer` once before `Start()` |
| **Every-frame** | Simulations, real-time effects (e.g., Game of Life, particles) | Re-call `PushComputeLayer` at the end of the callback |
| **Conditional** | Run compute only when inputs change | Call `PushComputeLayer` from inside `OnUpdate` when needed |

**Every-frame pattern** (used by the built-in examples):
```cpp
std::function<void()> loop;
loop = [&]() {
  engine.PushComputeLayer(handle, [&](MyResult result) {
    // process result ...
    loop(); // re-queue for next frame
  });
};
loop(); // start the chain
engine.Start();
```

---

### Displaying Compute Results as a Texture

When a compute layer writes to a `wgpu::Texture`, pass it to a `TextureRenderLayer` and call `Draw` in `OnUpdate`:

```cpp
#include "lib/render_layer/TextureRenderLayer.hpp"

wglib::render_layers::TextureRenderLayer display{1280, 720};

std::function<void()> loop;
loop = [&]() {
  engine.PushComputeLayer(handle, [&](wgpu::Texture tex) {
    display.setTexture(std::move(tex));
    loop();
  });
};

engine.OnUpdate([&](float) { engine.Draw(display); });
loop();
engine.Start();
```

---

## Render Layers

The library provides three built-in render layer types:

- **RectangleRenderLayer**: Renders filled rectangles
  - Constructor: `RectangleRenderLayer(glm::vec2 position, glm::vec2 size, glm::vec3 color)`
  - Methods: `setPosition()`, `setSize()`, `setColor()`, `getPosition()`, `getSize()`, `getColor()`

- **CircleRenderLayer**: Renders filled circles
  - Constructor: `CircleRenderLayer(glm::vec2 origin, float radius, glm::vec3 color, uint32_t resolution = 50)`
  - Methods: `setOrigin()`, `setRadius()`, `setColor()`, `setResolution()`, `getOrigin()`, `getRadius()`, `getColor()`, `getResolution()`

- **TextureRenderLayer**: Renders a GPU texture to the screen
  - Constructor: `TextureRenderLayer(float width, float height)` or `TextureRenderLayer(wgpu::Texture* texture, float width, float height)`
  - Methods: `setTexture(wgpu::Texture)`, `getTexture()`
  - Primary use case: displaying the output of a compute shader

### Creating Custom Render Layers

Inherit from `RenderLayer` and implement:
- `Render(wgpu::RenderPassEncoder&)` — encode draw calls
- `InitRes(wgpu::Device&, wgpu::TextureFormat, wgpu::BindGroupLayout&)` — allocate GPU resources
- `UpdateRes(wgpu::Device&)` — upload per-frame data (called each frame)

---

## How It Works

### Architecture

wglib is built on several core components:

1. **Engine** (`CoreEngine.hpp/cpp`): The main orchestrator that manages the update loop, rendering, and compute operations.
   - Initializes the WebGPU device and adapter
   - Creates the window through `WindowManager`
   - Coordinates rendering via `Renderer`
   - Manages compute operations through `ComputeEngine`
   - Provides frame rate control with `SetTargetFPS(double fps)`

2. **Renderer** (`CoreRenderer.hpp/cpp`): Manages the rendering pipeline.
   - Builds and caches render pipelines per layer type
   - Encodes render passes each frame
   - Maintains uniform buffers for screen size
   - Calls `Render()` on all layers queued via `engine.Draw()`

3. **WindowManager**: Platform abstraction for window creation.
   - GLFW on desktop
   - Emscripten canvas on the web
   - Handles surface creation and swapchain presentation

4. **ComputeEngine** (`ComputeEngine.hpp/cpp`): Manages GPU compute operations.
   - Holds a queue of `ComputeTask` entries (layer + completion callback)
   - Drains the queue at the start of each frame — each task creates a `CommandEncoder`, calls `layer->ComputeImpl()`, submits work, and registers an `OnSubmittedWorkDone` callback that calls `layer->getResult()` and forwards it to the user callback
   - `InitComputeLayer<T>(args...)` constructs the layer and calls `InitImpl` once

5. **ComputeLayer** (`ComputeLayer.hpp`): Abstract base for user-defined compute operations.
   - Templated on result type `T`
   - Exposes `ResultType` alias for type deduction
   - The three virtual methods (`InitImpl`, `ComputeImpl`, `getResultImpl`) are the only API surface users need to implement

6. **Render Layers**: Modular rendering units.
   - Each layer owns its GPU resources
   - Render pipeline instances are shared across layer instances of the same type

### Update Cycle

The engine runs a fixed-step update loop:

```
┌─────────────────────────────────┐
│  ComputeEngine::Compute()       │
│  Process all queued compute     │
│  tasks; register GPU callbacks  │
└──────────┬──────────────────────┘
           │
           ▼
┌─────────────────────────────────┐
│  ProcessEvents()                │
│  Deliver pending WebGPU         │
│  callbacks (compute results)    │
└──────────┬──────────────────────┘
           │
           ▼
┌─────────────────────────────────┐
│  Renderer::Render()             │
│  Draw all layers to the screen  │
└──────────┬──────────────────────┘
           │
           ▼
┌─────────────────────────────────┐
│  OnUpdate callback              │
│  User application logic;        │
│  call Draw() for next frame     │
└──────────┬──────────────────────┘
           │
           └──> (repeat)
```

Each frame:
1. Compute tasks are submitted to the GPU queue and callbacks are registered.
2. `ProcessEvents` delivers any completed GPU callbacks, including compute results.
3. Render layers queued via `engine.Draw()` are drawn to the screen.
4. The user's `OnUpdate` callback runs with `deltaTime` in seconds.

Because `ProcessEvents` runs **before** rendering, compute results from the current frame are available to `OnUpdate` in the same frame when using the every-frame chaining pattern.

### WebGPU Integration

- Uses Dawn for native desktop rendering
- Uses Emscripten's WebGPU implementation for web builds
- Shaders are written in WGSL (WebGPU Shading Language) and located in `src/shaders/`

---

## Project Structure

```
wglib/
├── src/
│   ├── main.cpp                      # Example application (4 demos)
│   ├── lib/
│   │   ├── CoreEngine.hpp/cpp        # Main engine
│   │   ├── CoreRenderer.hpp/cpp      # Rendering system
│   │   ├── CoreUtil.hpp              # Utility functions (logging, buffer helpers)
│   │   ├── WindowManager.*           # Window management (GLFW / Emscripten)
│   │   ├── render_layer/             # Render layer implementations
│   │   │   ├── RenderLayer.hpp       # Abstract base class
│   │   │   ├── RectangleRenderLayer.*
│   │   │   ├── CircleRenderLayer.*
│   │   │   ├── TextureRenderLayer.*
│   │   │   └── Vertex.hpp
│   │   └── compute/                  # Compute system
│   │       ├── ComputeEngine.*       # Task queue and execution
│   │       ├── ComputeLayer.hpp      # Templated abstract base
│   │       └── ExampleLayers/        # Built-in example compute layers
│   │           ├── ExampleLayer.hpp          # Array multiplication + CPU readback
│   │           ├── ConwaysGameOfLife.*        # Game of Life simulation
│   │           └── ParticleSimulation.*       # Physics particle system
│   └── shaders/                      # WGSL shader files
│       ├── default.wgsl              # Default shader for shapes
│       ├── texture.wgsl              # Texture rendering shader
│       ├── example.wgsl              # Compute shader for ExampleLayer
│       ├── ConwaysGameOfLife/
│       │   └── compute.wgsl
│       └── ParticleSimulation/
│           └── particle.wgsl
├── dawn/                             # Dawn WebGPU (submodule)
└── CMakeLists.txt                    # Build configuration
```

## Future TODO

- Event processing (keyboard, mouse)
- Manual Engine Tick Mode to control when the update cycle runs

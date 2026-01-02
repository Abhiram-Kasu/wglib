# wglib

A modern C++23 library for creating 2D graphics and compute applications using WebGPU. Build native desktop applications or compile to WebAssembly for the web.

## Features

- **Cross-platform rendering**: Write once, run on desktop (via Dawn) or web (via Emscripten)
- **WebGPU-based**: Modern GPU API with compute shader support
- **2D rendering primitives**: Built-in support for rectangles, circles, and textures
- **Compute layers**: Run GPU compute shaders for parallel processing
- **Update loop**: Simple callback-based update cycle for animations and game logic
- **Modern C++23**: Leverages latest C++ features for clean, expressive code

## Requirements

### Desktop Build
- CMake 3.22 or higher
- C++23 compatible compiler (GCC 13+, Clang 16+, or MSVC 2022+), Only tested with Clang however.
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
# Serve the result (use any web server)
mkdir build_web && cd build_web
emcmake cmake ..
cmake --build .
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
      glm::vec2{100, 100},        // position
      glm::vec2{200, 150},        // size
      glm::vec3{1.0f, 0.0f, 0.0f} // color (red)
  );
  
  wglib::render_layers::CircleRenderLayer circle(
      glm::vec2{400, 300},      // position
      75.0f,                     // radius
      glm::vec3{0.0f, 0.0f, 1.0f} // color (blue)
  );
  
  // Set up update callback
  engine.OnUpdate([&](const double deltaTime) {
    // Update logic goes here
    engine.Draw(rect);
    engine.Draw(circle);
  });
  
  // Start the main loop
  engine.Start();
}
```

### Using Compute Layers

Compute layers allow you to run GPU compute shaders for parallel processing. They execute asynchronously and can return results through callbacks.

```cpp
#include "lib/compute/ComputeLayer.hpp"

// Define your compute layer by inheriting from ComputeLayer
class MyComputeLayer : public wglib::compute::ComputeLayer {
protected:
  // Initialize GPU resources (buffers, pipelines, bind groups)
  // This pure virtual method must be implemented
  auto InitImpl(wgpu::Device& device) -> void override {
    // Create buffers, compute pipeline, bind groups
  }
  
  // Perform the compute operation
  // This pure virtual method must be implemented
  auto ComputeImpl(wgpu::CommandEncoder& encoder, wgpu::Queue& queue) -> void override {
    // Encode compute pass and submit work
  }
  
  // Return pointer to result data
  // This pure virtual method must be implemented
  auto getResultImpl() -> const void* override {
    // Return pointer to result (e.g., buffer data, texture)
    return &myResult;
  }
};

// In your main function:
MyComputeLayer myCompute{/* constructor args */};

// Push compute layer and get a handle
auto& handle = engine.PushComputeLayer(myCompute);

// Set up callback for when compute completes
handle.onComplete([](const void* result) {
  // Process results
  auto* data = reinterpret_cast<const float*>(result);
  // Use your data...
});
```

Compute layers are processed at the beginning of each frame, before rendering. You can queue multiple compute operations, and they will be executed in order.

### Render Layers

The library provides three built-in render layer types:

- **RectangleRenderLayer**: Renders filled rectangles
  - Constructor: `RectangleRenderLayer(glm::vec2 position, glm::vec2 size, glm::vec3 color)`
  - Methods: `setPosition()`, `setSize()`, `setColor()`, `getPosition()`, `getSize()`, `getColor()`
  
- **CircleRenderLayer**: Renders filled circles
  - Constructor: `CircleRenderLayer(glm::vec2 origin, float radius, glm::vec3 color, uint32_t resolution = 50)`
  - Methods: `setOrigin()`, `setRadius()`, `setColor()`, `setResolution()`, `getOrigin()`, `getRadius()`, `getColor()`, `getResolution()`

- **TextureRenderLayer**: Renders GPU textures to the screen
  - Constructor: `TextureRenderLayer(float width, float height)` or `TextureRenderLayer(wgpu::Texture* texture, float width, float height)`
  - Methods: `setTexture(wgpu::Texture*)`, `getTexture()`
  - Useful for displaying results from compute shaders or rendering framebuffers

#### Using TextureRenderLayer with Compute

TextureRenderLayer is particularly useful for visualizing compute shader output:

```cpp
#include "lib/render_layer/TextureRenderLayer.hpp"

// Create a texture render layer
wglib::render_layers::TextureRenderLayer textureLayer{800, 600};

// Push a compute layer that generates a texture
// The callback parameter is optional - can also use handle.onComplete() separately
engine.PushComputeLayer(myComputeLayer, [&](const void* data) {
  // Get the texture from compute result
  auto* texture = reinterpret_cast<const wgpu::Texture*>(data);
  textureLayer.setTexture(const_cast<wgpu::Texture*>(texture));
});

// Draw the texture
engine.OnUpdate([&](const double deltaTime) {
  engine.Draw(textureLayer);
});
```

#### Creating Custom Render Layers

You can create custom render layers by inheriting from `RenderLayer` and implementing:
- `Render(wgpu::RenderPassEncoder&)` - Rendering logic
- `InitRes(wgpu::Device&, wgpu::TextureFormat, wgpu::BindGroupLayout&)` - Resource initialization
- `UpdateRes(wgpu::Device&)` - Resource updates (called each frame if needed)

## How It Works

### Architecture

wglib is built on several core components:

1. **Engine** (`CoreEngine.hpp/cpp`): The main orchestrator that manages the update loop, rendering, and compute operations
   - Initializes WebGPU device and adapter
   - Manages window creation through WindowManager
   - Coordinates rendering via the Renderer
   - Handles compute operations through ComputeEngine
   - Provides frame rate control with `SetTargetFPS()`

2. **Renderer** (`CoreRenderer.hpp/cpp`): Manages the rendering pipeline
   - Creates and manages render pipelines for each layer type
   - Handles render pass encoding
   - Maintains uniform buffers for screen size and other global data
   - Calls `Render()` on all queued render layers each frame

3. **WindowManager**: Platform abstraction for window creation
   - Uses GLFW for desktop platforms
   - Integrates with Emscripten for web builds
   - Manages surface creation and presentation

4. **ComputeEngine**: Manages GPU compute operations
   - Executes compute shaders through ComputeLayer instances
   - Handles result callbacks asynchronously
   - Processes compute queue at the start of each frame
   - Returns compute results via callback mechanism

5. **Render Layers**: Modular rendering units
   - Each layer manages its own GPU resources (buffers, pipelines, bind groups)
   - Shared render pipelines across instances of the same layer type
   - Separate shader pipelines per layer type
   - Support for both geometry-based (rectangles, circles) and texture-based rendering

### Update Cycle

The engine runs a continuous update loop:

```
┌─────────────────────────────┐
│  Process Compute Layers     │
│  (GPU parallel processing)  │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Process WebGPU Events      │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Render Frame               │
│  (Draw all render layers)   │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Call Update Callback       │
│  (User application logic)   │
└──────────┬──────────────────┘
           │
           └──> (repeat)
```

Each frame:
1. Compute layers are processed first, allowing compute shaders to prepare data
2. WebGPU events are processed (device callbacks, errors, etc.)
3. Render layers are drawn to the screen
4. User's OnUpdate callback is invoked for game/application logic

### WebGPU Integration

- Uses Dawn for native desktop rendering
- Uses Emscripten's WebGPU implementation for web
- Shader code written in WGSL (WebGPU Shading Language)
- Shaders located in `src/shaders/`

## Project Structure

```
wglib/
├── src/
│   ├── main.cpp                 # Example application
│   ├── lib/
│   │   ├── CoreEngine.hpp/cpp   # Main engine
│   │   ├── CoreRenderer.hpp/cpp # Rendering system
│   │   ├── CoreUtil.hpp         # Utility functions
│   │   ├── WindowManager.*      # Window management
│   │   ├── render_layer/        # Render layer implementations
│   │   │   ├── RenderLayer.hpp
│   │   │   ├── RectangleRenderLayer.*
│   │   │   ├── CircleRenderLayer.*
│   │   │   ├── TextureRenderLayer.*
│   │   │   └── Vertex.hpp
│   │   └── compute/             # Compute system
│   │       ├── ComputeEngine.*
│   │       ├── ComputeLayer.hpp
│   │       └── ExampleLayers/
│   └── shaders/                 # WGSL shader files
│       ├── default.wgsl         # Default shader for shapes
│       ├── texture.wgsl         # Texture rendering shader
│       ├── example.wgsl         # Example compute shader
│       └── ConwaysGameOfLife/   # Conway's Game of Life compute shaders
├── dawn/                        # Dawn WebGPU (submodule)
├── scripts/
│   └── build_web.sh             # Web build script
└── CMakeLists.txt               # Build configuration
```

## Future TODO

- Event processing
- Manual Engine Tick Mode to control when the update cycle runs

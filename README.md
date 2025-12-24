# wglib

A modern C++23 library for creating 2D graphics and compute applications using WebGPU. Build native desktop applications or compile to WebAssembly for the web.

## Features

- **Cross-platform rendering**: Write once, run on desktop (via Dawn) or web (via Emscripten)
- **WebGPU-based**: Modern GPU API with compute shader support
- **2D rendering primitives**: Built-in support for rectangles and circles
- **Compute layers**: Run GPU compute shaders for parallel processing
- **Update loop**: Simple callback-based update cycle for animations and game logic
- **Modern C++23**: Leverages latest C++ features for clean, expressive code

## Requirements

### Desktop Build
- CMake 3.22 or higher
- C++23 compatible compiler (GCC 13+, Clang 16+, or MSVC 2022+)
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
git clone --recursive https://github.com/Abhiram-Kasu/wglib.git
cd wglib
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
# Run the build script
./scripts/build_web.sh

# Serve the result (use any web server)
cd build_web
python3 -m http.server 8000
# Open http://localhost:8000/wglib.html in your browser
```

## How to Use

### Basic Setup

```cpp
#include "lib/Engine.hpp"
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

Compute layers allow you to run GPU compute shaders for parallel processing:

```cpp
#include "lib/compute/ComputeLayer.hpp"

// Define your compute layer by inheriting from ComputeLayer
class MyComputeLayer : public wglib::compute::ComputeLayer {
  // Implement required methods:
  // - InitImpl(wgpu::Device&)
  // - ComputeImpl(wgpu::CommandEncoder&, wgpu::Queue&)
  // - getResultImpl()
};

// In your main function:
MyComputeLayer myCompute{/* constructor args */};
auto& handle = engine.PushComputeLayer(myCompute);

// Set up callback for when compute completes
handle.onComplete([](const void* buffer) {
  // Process results
  auto* data = reinterpret_cast<const float*>(buffer);
  // Use your data...
});
```

### Render Layers

The library provides two built-in render layer types:

- **RectangleRenderLayer**: Renders filled rectangles
  - Methods: `setPosition()`, `setSize()`, `setColor()`, `getPosition()`, `getSize()`
  
- **CircleRenderLayer**: Renders filled circles
  - Methods: `setPosition()`, `setRadius()`, `setColor()`, `getRadius()`

You can create custom render layers by inheriting from `RenderLayer` and implementing:
- `Render()` - Rendering logic
- `InitRes()` - Resource initialization
- `UpdateRes()` - Resource updates

## How It Works

### Architecture

wglib is built on several core components:

1. **Engine**: The main orchestrator that manages the update loop, rendering, and compute operations
   - Initializes WebGPU device and adapter
   - Manages window creation through WindowManager
   - Coordinates rendering via the Renderer
   - Handles compute operations through ComputeEngine

2. **Renderer**: Manages the rendering pipeline
   - Creates and manages render pipelines
   - Handles render pass encoding
   - Maintains bind groups for uniforms

3. **WindowManager**: Platform abstraction for window creation
   - Uses GLFW for desktop
   - Integrates with Emscripten for web

4. **ComputeEngine**: Manages GPU compute operations
   - Executes compute shaders
   - Handles result callbacks
   - Manages compute pipelines and buffers

5. **Render Layers**: Modular rendering units
   - Each layer manages its own GPU resources
   - Vertex/index buffers for geometry
   - Separate shader pipelines per layer type

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
│  Render Frame               │
│  (Draw all render layers)   │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Process WebGPU Events      │
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

### WebGPU Integration

- Uses Dawn for native desktop rendering
- Uses Emscripten's WebGPU implementation for web
- Shader code written in WGSL (WebGPU Shading Language)
- Shaders located in `src/shaders/`

## Project Structure

```
wglib/
├── src/
│   ├── main.cpp              # Example application
│   ├── lib/
│   │   ├── Engine.hpp/cpp    # Main engine
│   │   ├── Renderer.hpp/cpp  # Rendering system
│   │   ├── WindowManager.*   # Window management
│   │   ├── Util.hpp          # Utility functions
│   │   ├── render_layer/     # Render layer implementations
│   │   │   ├── RenderLayer.hpp
│   │   │   ├── RectangleRenderLayer.*
│   │   │   ├── CircleRenderLayer.*
│   │   │   └── Vertex.hpp
│   │   └── compute/          # Compute system
│   │       ├── ComputeEngine.*
│   │       ├── ComputeLayer.hpp
│   │       └── ExampleLayers/
│   └── shaders/              # WGSL shader files
├── dawn/                     # Dawn WebGPU (submodule)
├── scripts/
│   └── build_web.sh          # Web build script
└── CMakeLists.txt            # Build configuration
```

## Future TODO

- Event processing
- Manual Engine Tick Mode to control when the update cycle runs



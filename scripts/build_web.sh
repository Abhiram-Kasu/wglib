#!/bin/bash

# Exit on error
set -e

echo "Building wglib for WebAssembly..."

# Create build directory if it doesn't exist
if [ ! -d "build_web" ]; then
    mkdir build_web
fi

cd build_web

# Create the gen directories and copy required headers before CMake runs
echo "Creating required directories and copying headers..."
mkdir -p dawn/gen/src/emdawnwebgpu/include/webgpu
cp ../dawn/include/webgpu/webgpu_enum_class_bitmasks.h dawn/gen/src/emdawnwebgpu/include/webgpu/
cp ../dawn/include/webgpu/webgpu_glfw.h dawn/gen/src/emdawnwebgpu/include/webgpu/

# Configure with Emscripten
echo "Configuring with emcmake..."
emcmake cmake .. -G Ninja

# Build
echo "Building with ninja..."
ninja



echo "Build complete!"

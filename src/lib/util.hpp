//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once
#include "webgpu/webgpu_cpp.h"
#include "webgpu/webgpu_cpp_print.h"
#include <cstdlib> // for std::exit
#include <fstream> // for std::ifstream, std::ios::binary
#include <print>
#include <print> // if you use std::println
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace std {
// Formatter specifically for wgpu types
template <typename T>
  requires std::is_enum_v<T> && requires(std::ostream &os, const T &value) {
    { os << value } -> std::convertible_to<std::ostream &>;
  }
struct formatter<T> : formatter<string> {
  auto format(const T &value, format_context &ctx) const {
    std::stringstream oss;
    oss << value;
    return formatter<string>::format(oss.str(), ctx);
  }
};
} // namespace std
namespace wglib::util {
template <typename... Args>
void log(std::format_string<Args...> fmt, Args &&...args) {
  std::println(fmt, std::forward<Args>(args)...);
}

inline void log(std::string_view msg) { util::log("{}", msg); }
inline auto dispatchSizeCeil(size_t total_elements, size_t workgroup_size)
    -> size_t {
  if (workgroup_size == 0)
    return 0; // Avoid division by zero
  // The formula: (dividend + divisor - 1) / divisor
  return (total_elements + workgroup_size - 1) / workgroup_size;
}

inline auto readFile(std::string_view path) -> std::string {
#ifdef __EMSCRIPTEN__
  // For Emscripten, convert relative paths to absolute virtual filesystem paths
  std::string adjusted_path(path);
  if (adjusted_path.starts_with("../")) {
    adjusted_path = adjusted_path.substr(3); // Remove "../"
    adjusted_path = "/" + adjusted_path;
  }
  std::ifstream f(adjusted_path, std::ios::binary);
#else
  std::ifstream f(path.data(), std::ios::binary);
#endif
  if (!f) {
#ifdef __EMSCRIPTEN__
    util::log("Could not open file: {} (adjusted: {})", path, adjusted_path);
#else
    util::log("Could not open file: {}", path);
#endif
    std::exit(EXIT_FAILURE);
  }

  // Seek → size → read. Fast, minimal allocations.
  f.seekg(0, std::ios::end);
  const auto size = f.tellg();
  f.seekg(0, std::ios::beg);

  std::string contents;
  contents.resize(size);
  f.read(contents.data(), contents.size());

  return contents;
}

inline auto createShaderModuleFromFile(std::string_view path,
                                       const wgpu::Device &device)
    -> wgpu::ShaderModule {
  const auto shaderCode = readFile(path);
  wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode.c_str()}};
  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
  return device.CreateShaderModule(&shaderModuleDescriptor);
}

template <typename T, wgpu::BufferUsage Usage>
wgpu::Buffer createBuffer(const wgpu::Device &device, uint64_t count,
                          bool mappedAtCreation = false) {
  // Uniform buffers
  if constexpr (Usage & wgpu::BufferUsage::Uniform) {
    static_assert(alignof(T) >= 16,
                  "Uniform buffer types must be at least 16-byte aligned");
  }

  // Storage buffers
  if constexpr (Usage & wgpu::BufferUsage::Storage) {
    static_assert(alignof(T) >= 4,
                  "Storage buffer types must be at least 4-byte aligned");
  }

  // Vertex buffers
  if constexpr (Usage & wgpu::BufferUsage::Vertex) {
    static_assert(
        alignof(T) >= 4,
        "Vertex buffer element types must be at least 4-byte aligned");
  }

  auto size = sizeof(T) * count;

  // Uniform buffers must be padded to 16 bytes
  if constexpr (Usage & wgpu::BufferUsage::Uniform) {
    size = (size + 15) & ~uint64_t(15);
  }

  wgpu::BufferDescriptor desc{
      .size = size,
      .usage = Usage,
      .mappedAtCreation = mappedAtCreation,
  };

  return device.CreateBuffer(&desc);
}

} // namespace wglib::util

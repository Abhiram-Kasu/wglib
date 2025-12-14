//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once
#include <cstdlib>  // for std::exit
#include <fstream>  // for std::ifstream, std::ios::binary
#include <iterator> // for std::istreambuf_iterator
#include <print>
#include <print> // if you use std::println
#include <string>
#include <string_view>

namespace wglib::util {
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
    std::println("Could not open file: {} (adjusted: {})", path, adjusted_path);
#else
    std::println("Could not open file: {}", path);
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
} // namespace wglib::util

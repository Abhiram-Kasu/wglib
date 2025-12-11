//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once
#include <print>
#include <string>
#include <string>
#include <string_view>
#include <fstream>     // for std::ifstream, std::ios::binary
#include <iterator>    // for std::istreambuf_iterator
#include <print>       // if you use std::println
#include <cstdlib>     // for std::exit

namespace wglib::util
{
inline auto readFile(std::string_view path) -> std::string
{
    std::ifstream f(path.data(), std::ios::binary);
    if (!f)
    {
        std::println("Could not open file: {}", path);
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
} // wglib
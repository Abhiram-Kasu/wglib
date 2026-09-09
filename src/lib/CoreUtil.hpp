//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once
#include "webgpu/webgpu_cpp.h"
#include <array>
#include <bit>
#include <iostream>
#ifndef __EMSCRIPTEN__
#include "webgpu/webgpu_cpp_print.h"
#endif
#include <cstdlib> // for std::exit
#include <fstream> // for std::ifstream, std::ios::binary
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <print> // if you use std::println
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace std
{
#ifndef __EMSCRIPTEN__
template <typename T>
    requires std::is_enum_v<T> && requires(std::ostream &os, const T &value) {
        { os << value } -> std::convertible_to<std::ostream &>;
    }
struct formatter<T> : formatter<string>
{
    auto format(const T &value, format_context &ctx) const
    {
        std::stringstream oss;
        oss << value;
        return formatter<string>::format(oss.str(), ctx);
    }
};
#else
// Formatter for wgpu enums without operator<< support since emcc headers dont
// have webgpu_cpp_print.h
template <typename T>
    requires std::is_enum_v<T>
struct formatter<T> : formatter<std::underlying_type_t<T>>
{
    auto format(const T &value, format_context &ctx) const
    {
        return formatter<std::underlying_type_t<T>>::format(static_cast<std::underlying_type_t<T>>(value), ctx);
    }
};
#endif
} // namespace std
namespace wglib::util
{
template <typename... Args> void log(std::format_string<Args...> fmt, Args &&...args)
{
    std::println(fmt, std::forward<Args>(args)...);
}

inline void log(std::string_view msg)
{
    util::log("{}", msg);
}

template <typename T> inline auto divCeil(T dividend, T divisor) -> T
{
    if (divisor == 0)
        return 0; // Avoid division by zero
    // The formula: (dividend + divisor - 1) / divisor
    return (dividend + divisor - 1) / divisor;
}

inline auto readFile(std::string_view path) -> std::string
{
#ifdef __EMSCRIPTEN__
    // For Emscripten, convert relative paths to absolute virtual filesystem paths
    std::string adjusted_path(path);
    if (adjusted_path.starts_with("../"))
    {
        adjusted_path = adjusted_path.substr(3); // Remove "../"
        adjusted_path = "/" + adjusted_path;
    }
    std::ifstream f(adjusted_path, std::ios::binary);
#else
    std::ifstream f(path.data(), std::ios::binary);
#endif
    if (!f)
    {
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

inline auto createShaderModuleFromFile(std::string_view path, const wgpu::Device &device) -> wgpu::ShaderModule
{
    const auto shaderCode = readFile(path);
    wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode.c_str()}};
    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
    return device.CreateShaderModule(&shaderModuleDescriptor);
}

template <typename T, wgpu::BufferUsage Usage>
wgpu::Buffer createBuffer(const wgpu::Device &device, uint64_t count, bool mappedAtCreation = false)
{
    // Uniform buffers
    if constexpr (Usage & wgpu::BufferUsage::Uniform)
    {
        static_assert(alignof(T) >= 16, "Uniform buffer types must be at least 16-byte aligned");
    }

    // Storage buffers
    if constexpr (Usage & wgpu::BufferUsage::Storage)
    {
        static_assert(alignof(T) >= 4, "Storage buffer types must be at least 4-byte aligned");
    }

    // Vertex buffers
    if constexpr (Usage & wgpu::BufferUsage::Vertex)
    {
        static_assert(alignof(T) >= 4, "Vertex buffer element types must be at least 4-byte aligned");
    }

    auto size = sizeof(T) * count;

    // Uniform buffers must be padded to 16 bytes
    if constexpr (Usage & wgpu::BufferUsage::Uniform)
    {
        size = (size + 15) & ~uint64_t{15};
    }

    wgpu::BufferDescriptor desc{
        .usage = Usage,
        .size = size,
        .mappedAtCreation = mappedAtCreation,
    };

    return device.CreateBuffer(&desc);
}

static constexpr auto is_power_of_two(auto number) -> bool
{
    return std::has_single_bit(number);
}
template <size_t BufferSize, size_t BufferCount>
    requires(BufferSize > 0 and is_power_of_two(BufferSize) and BufferCount > 0)
struct StagingBelt
{
  private:
    struct State
    {
        std::array<wgpu::Buffer, BufferCount> buffers;
        // This is a ring of indices, not a ring of bytes.  The buffers are
        // fixed-size pool entries; the ring is just the free-list storage.
        std::array<size_t, BufferCount> free{};
        size_t head{0};
        size_t tail{0};
        size_t count{0};
        std::mutex mutex;

        explicit State(const wgpu::Device &device)
        {
            for (size_t i = 0; i < BufferCount; ++i)
            {
                buffers[i] = util::createBuffer<std::byte, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc |
                                                               wgpu::BufferUsage::MapWrite>(device, BufferSize, true);
                free[i] = i;
            }
            count = BufferCount;
        }

        auto try_acquire() -> std::optional<size_t>
        {
            std::scoped_lock lock(mutex);
            if (count == 0)
                return std::nullopt;

            const auto index = free[head];
            head = (head + 1) % BufferCount;
            --count;
            return index;
        }

        void release(size_t index)
        {
            std::scoped_lock lock(mutex);
            // A successful map callback is the only path that reaches here,
            // so a slot cannot be released twice by a live handle.
            free[tail] = index;
            tail = (tail + 1) % BufferCount;
            ++count;
        }
    };

    std::shared_ptr<State> m_state;

  public:
    template <wgpu::BufferMapState MapState> struct BeltHandle
    {
      private:
        std::shared_ptr<State> state;
        size_t index{};

        BeltHandle(std::shared_ptr<State> state_, size_t index_) : state(std::move(state_)), index(index_)
        {
        }

        void return_mapped_buffer()
        {
            auto state_to_keep_alive = state;
            state->buffers[index].MapAsync(
                wgpu::MapMode::Write, 0, BufferSize, wgpu::CallbackMode::AllowSpontaneous,
                [state_to_keep_alive, index = index](wgpu::MapAsyncStatus status, wgpu::StringView error) {
                    if (status == wgpu::MapAsyncStatus::Success)
                    {
                        state_to_keep_alive->release(index);
                    }
                    else
                    {
                        util::log("Staging buffer {} could not be remapped: {}", index, error.data);
                    }
                });
        }

      private:
        friend struct StagingBelt;

      public:
        BeltHandle() = delete;
        BeltHandle(const BeltHandle &) = delete;
        auto operator=(const BeltHandle &) -> BeltHandle & = delete;
        BeltHandle(BeltHandle &&other) noexcept : state(std::exchange(other.state, nullptr)), index(other.index)
        {
        }
        auto operator=(BeltHandle &&other) noexcept -> BeltHandle &
        {
            if (this != &other)
            {
                reset();
                state = std::exchange(other.state, nullptr);
                index = other.index;
            }
            return *this;
        }

        auto write_to_buffer(
            std::invocable<const wgpu::Buffer &> auto func) && -> BeltHandle<wgpu::BufferMapState::Unmapped>
            requires(MapState == wgpu::BufferMapState::Mapped)
        {
            func(state->buffers[index]);
            state->buffers[index].Unmap();
            return {std::move(state), index};
        }

        operator const wgpu::Buffer &() const
            requires(MapState == wgpu::BufferMapState::Unmapped)
        {
            return state->buffers[index];
        }

        auto get() const -> const wgpu::Buffer &
            requires(MapState == wgpu::BufferMapState::Unmapped)
        {
            return state->buffers[index];
        }

        void reset()
        {
            if (!state)
                return;

            if constexpr (MapState == wgpu::BufferMapState::Mapped)
                state->release(index);
            else
                return_mapped_buffer();

            state.reset();
        }

        ~BeltHandle()
        {
            reset();
        }
    };

    StagingBelt(const wgpu::Device &device) : m_state(std::make_shared<State>(device))
    {
    }

    StagingBelt(const StagingBelt &) = delete;
    auto operator=(const StagingBelt &) -> StagingBelt & = delete;

    auto acquire() -> std::optional<BeltHandle<wgpu::BufferMapState::Mapped>>
    {
        if (auto index = m_state->try_acquire())
            return BeltHandle<wgpu::BufferMapState::Mapped>{m_state, *index};
        return std::nullopt;
    }
};

} // namespace wglib::util

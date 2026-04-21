#pragma once
#include "GLFW/glfw3.h"
#include "WindowManager.hpp"
#include "compute/ComputeEngine.hpp"
#include "lib/compute/ComputeLayer.hpp"
#include "lib/render_layer/RenderLayer.hpp"
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <webgpu/webgpu_cpp.h>

#include "CoreRenderer.hpp"

namespace wglib
{
class Engine
{
    std::unique_ptr<WindowManager> m_window_manager;
    std::unique_ptr<wglib::compute::ComputeEngine> m_computeEngine;
    wgpu::Instance m_instance;
    wgpu::Device m_device;
    wgpu::Adapter m_adapter;
    glm::vec2 m_window_size;
    std::unique_ptr<Renderer> m_renderer;
    std::function<void(double)> m_update_function;
    double m_last_frame_time{0.0f};
    double m_target_frame_time{0.0};
    double m_accumulator{0.0};

    auto render() -> void;
    auto update_frame(double delta) -> void;

  public:
    Engine(glm::vec2 size, std::string_view title);

    ~Engine();

    auto GetWindow() -> GLFWwindow *
    {
        return m_window_manager->window();
    }

    auto Start() -> void;

    auto OnUpdate(std::function<void(float)> &&function) -> void
    {
        m_update_function = function;
    }

    auto SetTargetFPS(double fps) -> void
    {
        m_target_frame_time = fps > 0.0 ? 1.0 / fps : 0.0;
    }
    template <std::derived_from<render_layers::RenderLayer> T> auto Draw(Renderer::Ref<T> renderLayer) -> void
    {
        m_renderer->pushRenderLayer(renderLayer);
    }

    template <std::derived_from<compute::IComputeLayer> LayerType, typename... Args>
    auto InitComputeLayer(Args &&...args) -> compute::ComputeEngine::ComputeLayerHandle<typename LayerType::ResultType>
    {
        return m_computeEngine->InitComputeLayer<LayerType>(std::forward<Args>(args)...);
    }
    template <typename TResult, std::invocable<TResult> CB>
    auto PushComputeLayer(compute::ComputeEngine::ComputeLayerHandle<TResult> &handle, CB &&onComplete) -> void
    {

        m_computeEngine->PushComputeLayer(handle, std::forward<CB>(onComplete));
    }
    template <std::derived_from<render_layers::RenderLayer> T, typename... Args> auto CreateRenderLayer(Args &&...args)
    {
        return m_renderer->CreateRenderLayer<T>(std::forward<Args>(args)...);
    }
};
} // namespace wglib

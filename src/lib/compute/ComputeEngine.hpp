#pragma once

#include "ComputeLayer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <concepts>
#include <functional>
#include <memory>
#include <queue>
#include <type_traits>
#include <utility>

namespace wglib {
class Engine;
}

namespace wglib::compute {

class ComputeEngine {

public:
  template <typename TResult> struct ComputeLayerHandle {
    friend class ComputeEngine;

  private:
    std::shared_ptr<ComputeLayer<TResult>> m_compute_layer;

    ComputeLayerHandle(std::shared_ptr<ComputeLayer<TResult>> layer)
        : m_compute_layer(std::move(layer)) {}
  };

private:
  struct ComputeTask {
    std::shared_ptr<IComputeLayer> layer;
    std::function<void()> onComplete;
  };

private:
  wgpu::Device &m_device;
  std::queue<ComputeTask> m_computeQueue;

  friend class wglib::Engine;
  // Called every tick by the engine
  auto Compute() -> void;

public:
  ComputeEngine(wgpu::Device &m_device);

  template <std::derived_from<IComputeLayer> LayerType, typename... Args>
  auto InitComputeLayer(Args &&...args)
      -> ComputeLayerHandle<typename LayerType::ResultType> {

    auto layer = std::make_shared<LayerType>(std::forward<Args>(args)...);
    layer->Init(m_device);
    return ComputeLayerHandle<typename LayerType::ResultType>{std::move(layer)};
  }

  template <typename TResult, std::invocable<TResult> CB>
  auto PushComputeLayer(ComputeLayerHandle<TResult> &handle, CB &&onComplete)
      -> void {

    auto completion = [layer = handle.m_compute_layer,
                       cb = std::move(onComplete)]() mutable {
      auto result = layer->getResult();
      cb(std::move(result));
    };

    m_computeQueue.push(
        ComputeTask{handle.m_compute_layer, std::move(completion)});
  }
};

} // namespace wglib::compute

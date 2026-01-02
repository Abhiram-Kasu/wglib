#pragma once

#include "ComputeLayer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <functional>
#include <queue>

// Use case :
// auto handle = engine::pushComputeLayer<ExampleComputeLayer>(args...)
// handle.OnComplete([&](auto res){
//  engine.DoSomething with resuly
//  or just print result?
// })
namespace wglib {
class Engine;
}
namespace wglib::compute {

class ComputeEngine {

  struct ComputeHandle {
    friend class ComputeEngine;

  private:
    std::optional<std::function<void(const void *)>> m_onComplete;
    ComputeLayer &computeLayer;

  public:
    auto onComplete(std::function<void(const void *)> cb) -> void {
      m_onComplete = cb;
    }
    ComputeHandle(
        ComputeLayer &computeLayer,
        const std::optional<std::function<void(const void *)>> &onComplete)
        : m_onComplete(onComplete), computeLayer(computeLayer) {}
  };

private:
  wgpu::Device &m_device;
  std::queue<ComputeHandle> m_computeQueue;

  friend class wglib::Engine;
  // Called every tick by the engine
  auto Compute() -> void;

public:
  ComputeEngine(wgpu::Device &m_device);
  auto PushComputeLayer(ComputeLayer &computeLayer,
                        std::optional<std::function<void(const void *)>>
                            onComplete = std::nullopt) -> ComputeHandle &;
};
} // namespace wglib::compute

#include "ComputeEngine.hpp"
#include "lib/Util.hpp"
#include "webgpu/webgpu_cpp.h"
#include <optional>

namespace wglib::compute {
ComputeEngine::ComputeEngine(wgpu::Device &device) : m_device(device) {}

auto ComputeEngine::PushComputeLayer(
    ComputeLayer &computeLayer,
    std::optional<std::function<void(const void *)>> onComplete)
    -> ComputeHandle & {
  computeLayer.Init(m_device);

  m_computeQueue.push(ComputeHandle{computeLayer, onComplete});
  return m_computeQueue.front();
}

auto ComputeEngine::Compute() -> void {
  auto queue = m_device.GetQueue();

  while (not m_computeQueue.empty()) {
    auto &handle = m_computeQueue.front();
    auto commandEncoder = m_device.CreateCommandEncoder();
    handle.computeLayer.Compute(commandEncoder, queue);

    auto userCallback = handle.m_onComplete;
    auto &computeLayerRef = handle.computeLayer;

    m_computeQueue.pop();

    queue.OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowProcessEvents,
        [userCallback, &computeLayerRef](wgpu::QueueWorkDoneStatus status,
                                         wgpu::StringView error) {
          if (status == wgpu::QueueWorkDoneStatus::Success) {
            if (auto cb = userCallback.value_or(nullptr)) {
              const void *result = computeLayerRef.getResult();
              cb(result);
            }
          } else {
            util::log("Compute work failed: {}", error.data);
          }
        });
  }
}

} // namespace wglib::compute

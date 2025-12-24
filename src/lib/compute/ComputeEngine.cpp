#include "ComputeEngine.hpp"
#include "lib/Util.hpp"
#include "webgpu/webgpu_cpp.h"
#include <optional>

namespace wglib::compute {
ComputeEngine::ComputeEngine(wgpu::Device &device) : m_device(device) {}
auto ComputeEngine::PushComputeLayer(ComputeLayer &computeLayer)
    -> ComputeHandle & {
  // Init the compute layer if not already inited
  computeLayer.Init(m_device);

  m_computeQueue.push(ComputeHandle{computeLayer, std::nullopt});
  return m_computeQueue.front();
}

auto ComputeEngine::Compute() -> void {
  auto queue = m_device.GetQueue();

  while (not m_computeQueue.empty()) {
    util::log("Processing queue item");
    auto &handle = m_computeQueue.front();
    auto commandEncoder = m_device.CreateCommandEncoder();
    handle.computeLayer.Compute(commandEncoder, queue);
    m_computeQueue.pop();
    queue.OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowSpontaneous,
        [&](wgpu::QueueWorkDoneStatus status, wgpu::StringView error) {
          if (status == wgpu::QueueWorkDoneStatus::Success) {
            util::log("Queued Work Success");
            if (auto cb = handle.m_onComplete.value_or(nullptr)) {
              util::log("Callback called");

              cb(handle.computeLayer.getResult());
            } else {
              util::log("No callback registered");
            }
          } else {
            util::log("Queued work failed: {}", error.data);
          }
        });
    util::log("queue work callback registered");
  }
}

} // namespace wglib::compute

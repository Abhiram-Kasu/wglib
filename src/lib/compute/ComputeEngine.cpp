#include "ComputeEngine.hpp"
#include "lib/CoreUtil.hpp"
#include "webgpu/webgpu_cpp.h"
#include <optional>

namespace wglib::compute {
ComputeEngine::ComputeEngine(wgpu::Device &device) : m_device(device) {}

auto ComputeEngine::Compute() -> void {
  auto queue = m_device.GetQueue();

  while (not m_computeQueue.empty()) {
    auto task = std::move(m_computeQueue.front());

    m_computeQueue.pop();

    auto commandEncoder = m_device.CreateCommandEncoder();
    task.layer->Compute(commandEncoder, queue);

    queue.OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowProcessEvents,
        [callback = std::move(task.onComplete),
         keepAlive = std::move(task.layer)](wgpu::QueueWorkDoneStatus status,
                                            wgpu::StringView error) {
          if (status == wgpu::QueueWorkDoneStatus::Success) {
            if (callback) {
              callback();
            }
          } else {
            util::log("Compute work failed: {}", error.data);
          }
        });
  }
}

} // namespace wglib::compute

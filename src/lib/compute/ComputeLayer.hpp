#pragma once
#include "webgpu/webgpu_cpp.h"
namespace wglib::compute {
class ComputeLayer {
  friend class ComputeEngine;

private:
  virtual auto getResult() -> const void * final {
    return this->getResultImpl();
  }
  virtual auto Init(wgpu::Device &d) -> void final { this->InitImpl(d); }
  virtual auto Compute(wgpu::CommandEncoder &e, wgpu::Queue &q) -> void final {
    this->ComputeImpl(e, q);
  }

protected:
  virtual auto getResultImpl() -> const void * = 0;
  virtual auto InitImpl(wgpu::Device &) -> void = 0;
  virtual auto ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &) -> void = 0;
};
} // namespace wglib::compute

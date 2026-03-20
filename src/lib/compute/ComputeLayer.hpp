#pragma once
#include "webgpu/webgpu_cpp.h"
namespace wglib::compute {
// Type Erased Interface for ComputeLayer
class IComputeLayer {
public:
  virtual ~IComputeLayer() = default;
  virtual auto Init(wgpu::Device &) -> void = 0;
  virtual auto Compute(wgpu::CommandEncoder &, wgpu::Queue &) -> void = 0;
};

template <typename T> class ComputeLayer : public IComputeLayer {
  friend class ComputeEngine;

public:
  using ResultType = T;

private:
  virtual auto getResult() -> T final { return this->getResultImpl(); }
  virtual auto Init(wgpu::Device &d) -> void final { this->InitImpl(d); }
  virtual auto Compute(wgpu::CommandEncoder &e, wgpu::Queue &q) -> void final {
    this->ComputeImpl(e, q);
  }

protected:
  virtual auto getResultImpl() -> T = 0;
  virtual auto InitImpl(wgpu::Device &) -> void = 0;
  virtual auto ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &) -> void = 0;
};
} // namespace wglib::compute

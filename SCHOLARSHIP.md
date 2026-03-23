# HRT CppCon Scholarship — Project Description

**Question:** Describe an interesting C++ project you've worked on and its impact.

---

wglib is a modern C++23 library I built for creating 2D graphics and GPU compute
applications using WebGPU. The central design goal was write-once, run-everywhere:
a single codebase that targets both native desktop (via Google's Dawn) and
WebAssembly (via Emscripten), with zero platform-specific code required in user
applications.

The most technically interesting aspect is the templated `ComputeLayer<T>` system.
By encoding result types at compile time and using C++23 concepts to constrain
callbacks, the library eliminates runtime type casting entirely — GPU compute
results are delivered with full type safety through
`std::optional<std::span<const T, N>>` callbacks. This pattern made building
complex simulations like Conway's Game of Life and a particle physics engine
(10,000+ particles with O(N²) repulsion forces computed in WGSL shaders) feel
natural and safe.

The impact has been tangible: the library reduces the boilerplate required to write
a WebGPU application from hundreds of lines to fewer than twenty, while preserving
direct access to GPU primitives for performance-critical paths. Simulations built
with wglib run identically in a browser and natively, demonstrating that
high-performance C++ doesn't have to sacrifice portability.

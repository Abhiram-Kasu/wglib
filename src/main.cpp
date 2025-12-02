#include "lib/engine.hpp"
#include <GLFW/glfw3.h>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <vector>

#include "lib/render_layer/rectangle_render_layer.hpp"
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

wgpu::Instance instance;
wgpu::Adapter adapter;
wgpu::Device device;
wgpu::RenderPipeline pipeline;

wgpu::Surface surface;
wgpu::TextureFormat format;
const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

void ConfigureSurface() {
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    format = capabilities.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = device, .format = format, .width = kWidth, .height = kHeight
    };
    surface.Configure(&config);
}

void Init() {
    static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instanceDesc{
        .requiredFeatureCount = 1,
        .requiredFeatures = &kTimedWaitAny
    };
    instance = wgpu::CreateInstance(&instanceDesc);

    wgpu::Future f1 = instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter a,
           wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cout << "RequestAdapter: " << message << "\n";
                exit(0);
            }
            adapter = std::move(a);
        });
    instance.WaitAny(f1, UINT64_MAX);

    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback([](const wgpu::Device &,
                                       wgpu::ErrorType errorType,
                                       wgpu::StringView message) {
        std::cout << "Error: " << errorType << " - message: " << message << "\n";
    });

    wgpu::Future f2 = adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestDeviceStatus status, wgpu::Device d,
           wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::cout << "RequestDevice: " << message << "\n";
                exit(0);
            }
            device = std::move(d);
        });
    instance.WaitAny(f2, UINT64_MAX);
}

auto readFile(const char *filename) -> std::string {
    std::ifstream f{filename};
    if (!f) {
        std::cerr << "Could not open the file " << filename << "\n";
        exit(1);
    }
    std::string contents((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return contents;
}

struct Vertex_ {
    float position[2];
    float color[3];
};

wgpu::Buffer vertexBuffer;

wgpu::VertexBufferLayout vertexBufferLayout{
    .stepMode = wgpu::VertexStepMode::Vertex,
    .arrayStride = sizeof(Vertex_),
    .attributeCount = 2,

    .attributes =
    (wgpu::VertexAttribute[]){
        {
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = sizeof(float) * 2,
            .shaderLocation = 1,
        },
    },
};
std::vector<Vertex_> vertices{
    {{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
};

void CreateRenderPipeline() {
    vertexBuffer = device.CreateBuffer([] {
        static wgpu::BufferDescriptor desc{
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = sizeof(Vertex_) * 3,
            .mappedAtCreation = true,
        };
        return &desc;
    }());

    vertexBuffer.WriteMappedRange(0, vertices.data(),
                                  sizeof(Vertex_) * vertices.size());
    vertexBuffer.Unmap();

    const auto shaderCode = readFile("../src/shaders/triangle.wgsl");

    wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode.c_str()}};

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
    wgpu::ShaderModule shaderModule =
            device.CreateShaderModule(&shaderModuleDescriptor);

    wgpu::ColorTargetState colorTargetState{.format = format};

    wgpu::FragmentState fragmentState{
        .module = shaderModule, .targetCount = 1, .targets = &colorTargetState
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .vertex = {
            .module = shaderModule,
            .buffers = &vertexBufferLayout,
            .bufferCount = 1
        },
        .fragment = &fragmentState
    };
    pipeline = device.CreateRenderPipeline(&descriptor);
}

void Render() {
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    static uint64_t time = 0;

    time++;

    wgpu::RenderPassColorAttachment attachment{
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store
    };

    wgpu::RenderPassDescriptor renderpass{
        .colorAttachmentCount = 1,
        .colorAttachments = &attachment
    };

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    // change colors of vertices - each vertex cycles independently through red,
    // green, blue
    uint8_t index = 0;
    for (auto &vertex: vertices) {
        // Each vertex gets a phase offset so they cycle at different times
        float phase = index * 2.0944f; // 120 degrees offset per vertex
        // Red channel cycles with its own frequency
        vertex.color[0] = (sin((time) * 0.1f + phase) + 1.0f) * 0.5f;
        // Green channel cycles with phase offset of 2π/3 (120 degrees)
        vertex.color[1] = (sin((time) * 0.1f + phase + 2.0944f) + 1.0f) * 0.5f;
        // Blue channel cycles with phase offset of 4π/3 (240 degrees)
        vertex.color[2] = (sin((time) * 0.1f + phase + 4.1888f) + 1.0f) * 0.5f;
        index++;
    }
    encoder.WriteBuffer(vertexBuffer, 0,
                        reinterpret_cast<const uint8_t *>(vertices.data()),
                        vertices.size() * sizeof(Vertex_));
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    pass.SetPipeline(pipeline);
    pass.SetVertexBuffer(0, vertexBuffer);
    pass.Draw(3);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

void InitGraphics() {
    ConfigureSurface();
    CreateRenderPipeline();
}

void Start() {
    if (!glfwInit()) {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window =
            glfwCreateWindow(kWidth, kHeight, "WebGPU window", nullptr, nullptr);
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

    InitGraphics();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(Render, 0, false);
#else
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Render();
        surface.Present();
        instance.ProcessEvents();
    }
#endif
}

int main() {
    wglib::Engine engine({500, 500}, "title");


    engine.Draw<wglib::render_layers::RectangleRenderLayer>(glm::vec2{500, 500}, glm::vec2{100, 100},
                                                            glm::vec3{1.0f, 0.0f, 0.0f});
    Init();
    Start();
}
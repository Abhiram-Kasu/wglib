#include <__ostream/print.h>
#include <any>
#include <limits>
#include <numbers>
#include <ranges>
#include <span>
#include <thread>
#include <type_traits>

#include "GLFW/glfw3.h"
#include "glm/common.hpp"
#include "lib/CoreEngine.hpp"
#include "lib/CoreUtil.hpp"

#include "lib/compute/ExampleLayers/ConwaysGameOfLife.hpp"
#include "lib/compute/ExampleLayers/ExampleLayer.hpp"
#include "lib/compute/ExampleLayers/ParticleSimulation.hpp"
#include "lib/render_layer/CircleRenderLayer.hpp"
#include "lib/render_layer/RectangleRenderLayer.hpp"
#include "lib/render_layer/TextureRenderLayer.hpp"
#include "webgpu/webgpu_cpp.h"

auto runComputeAndDrawingExample() {
  wglib::Engine engine({500, 500}, "title");

  wglib::render_layers::RectangleRenderLayer rect1(
      glm::vec2{0, 0}, glm::vec2{100, 100}, glm::vec3{0.0f, 1.0f, 0.0f});

  wglib::render_layers::CircleRenderLayer circle(glm::vec2{250, 250}, 50.0f,
                                                 glm::vec3{0.0f, 0.0f, 1.0f});

  wglib::compute::ExampleLayer exampleLayer{50000, std::numbers::pi};
  auto &handle = engine.PushComputeLayer(exampleLayer);

  handle.onComplete([](const void *buffer) {
    auto *items = reinterpret_cast<const float *>(buffer);

    std::span<const float, 50'000> span(items, 50'000);

    for (const auto &item : span | std::ranges::views::take(10)) {
      wglib::util::log("Item: {}", item);
    }
  });
  engine.OnUpdate([&](const double s) {
    engine.Draw(rect1);
    static auto velocity = glm::vec2{50};
    if (rect1.getPosition().x + rect1.getSize().x > 500 or
        rect1.getPosition().x < 0) {
      velocity.x *= -1;
    }
    if (rect1.getPosition().y + rect1.getSize().y > 500 or
        rect1.getPosition().y < 0) {
      velocity.y *= -1;
    }

    rect1.setPosition(rect1.getPosition() + velocity * static_cast<float>(s));

    engine.Draw(circle);
    auto radius = circle.getRadius() - circle.getRadius() * s;
    if (radius <= 10)
      radius = 100;
    circle.setRadius(radius);

    auto time = glfwGetTime();
    auto rotating_color =
        glm::vec3((sin(time * 1.0f) + 1.0f) * 0.5f, // Red channel
                  (sin(time * 1.3f) + 1.0f) * 0.5f, // Green channel
                  (sin(time * 1.7f) + 1.0f) * 0.5f  // Blue channel
        );
    circle.setColor(rotating_color);
  });

  engine.Start();
}

auto runConwaysGameOfLife() {
  wglib::Engine engine({2560, 1440}, "title");
  wglib::compute::ConwaysGameOfLifeComputeLayer compute{{2560, 1440}};
  wglib::render_layers::TextureRenderLayer textureRenderLayer{2560, 1440};

  engine.SetTargetFPS(120.0);

  std::function<void()> runIteration;

  runIteration = [&]() {
    engine.PushComputeLayer(compute, [&](const void *data) {
      auto texture = reinterpret_cast<const wgpu::Texture *>(data);
      textureRenderLayer.setTexture(const_cast<wgpu::Texture *>(texture));
      runIteration();
    });
  };

  engine.OnUpdate([&](const double) { engine.Draw(textureRenderLayer); });

  runIteration();

  engine.Start();
}
auto runParticleSimulation() {
  wglib::Engine engine({2560, 1440}, "title");
  wglib::compute::ParticleSimulationLayer particleSimulationLayer(
      2000, {2560, 1440}, 2, {1, 0, 0, 1}, {500, 500}, 100, 0.016, 100, 0.98,
      2000, 50);
  wglib::render_layers::TextureRenderLayer textureRenderLayer{2560, 1440};

  engine.SetTargetFPS(120.0);

  std::function<void()> runIteration;

  runIteration = [&]() {
    engine.PushComputeLayer(particleSimulationLayer, [&](const void *data) {
      auto texture = reinterpret_cast<const wgpu::Texture *>(data);
      textureRenderLayer.setTexture(const_cast<wgpu::Texture *>(texture));
      runIteration();
    });
  };

  engine.OnUpdate([&](const double) { engine.Draw(textureRenderLayer); });

  runIteration();

  engine.Start();
}
int main() { runParticleSimulation(); }

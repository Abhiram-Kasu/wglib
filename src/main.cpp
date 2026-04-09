#include <numbers>
#include <ranges>
#include <span>
#include <string>

#include "GLFW/glfw3.h"
#include "lib/CoreEngine.hpp"
#include "lib/CoreUtil.hpp"

#include "lib/compute/ExampleLayers/ConwaysGameOfLife.hpp"
#include "lib/compute/ExampleLayers/ExampleLayer.hpp"
#include "lib/compute/ExampleLayers/ParticleSimulation.hpp"
#include "lib/render_layer/CircleRenderLayer.hpp"
#include "lib/render_layer/RectangleRenderLayer.hpp"
#include "lib/render_layer/TextureRenderLayer.hpp"
#include "webgpu/webgpu_cpp.h"

auto runComputeAndDrawingExample()
{
    constexpr auto height = 1440uz;
    constexpr auto width = 1440uz;
    wglib::Engine engine(glm::vec2{height, width}, "title");

    wglib::render_layers::RectangleRenderLayer rect1(glm::vec2{10, 10}, glm::vec2{300, 300},
                                                     glm::vec3{0.0f, 1.0f, 0.0f});

    wglib::render_layers::CircleRenderLayer circle(glm::vec2{250, 250}, 50.0f, glm::vec3{0.0f, 0.0f, 1.0f});

    auto compute = engine.InitComputeLayer<wglib::compute::ExampleLayer<50000>>(static_cast<float>(std::numbers::pi));

    engine.PushComputeLayer(compute, [](std::optional<std::span<const float, 50000>> res) {
        if (not res)
        {
            wglib::util::log("failed to get items");
        }
        else
        {
            auto span = *res;
            for (const auto &item : span | std::ranges::views::take(10))
            {
                wglib::util::log("Item: {}", item);
            }
        }
    });

    engine.OnUpdate([&](const double s) {
        engine.Draw(rect1);
        static auto velocity = glm::vec2{50};
        if (rect1.getPosition().x + rect1.getSize().x > width or rect1.getPosition().x < 0)
        {
            velocity.x *= -1;
        }
        if (rect1.getPosition().y + rect1.getSize().y > height or rect1.getPosition().y < 0)
        {
            velocity.y *= -1;
        }

        rect1.setPosition(rect1.getPosition() + velocity * static_cast<float>(s));

        engine.Draw(circle);
        auto radius = circle.getRadius() - circle.getRadius() * s;
        if (radius <= 10)
            radius = 100;
        circle.setRadius(radius);

        auto time = glfwGetTime();
        auto rotating_color = glm::vec3((sin(time * 1.0f) + 1.0f) * 0.5f, // Red channel
                                        (sin(time * 1.3f) + 1.0f) * 0.5f, // Green channel
                                        (sin(time * 1.7f) + 1.0f) * 0.5f  // Blue channel
        );
        circle.setColor(rotating_color);
    });

    engine.Start();
}

auto runConwaysGameOfLife()
{
    wglib::Engine engine({2560, 1440}, "title");

    auto compute = engine.InitComputeLayer<wglib::compute::ConwaysGameOfLifeComputeLayer>(glm::vec2{2560, 1440});
    wglib::render_layers::TextureRenderLayer textureRenderLayer{2560, 1440};

    engine.SetTargetFPS(120.0);

    auto ready = true;
    engine.OnUpdate([&](auto) {
        if (ready)
        {
            ready = false;
            engine.PushComputeLayer(compute, [&](wgpu::Texture texture) {
                textureRenderLayer.setTexture(texture);
                ready = true;
            });
        }
        engine.Draw(textureRenderLayer);
    });

    engine.Start();
}

auto runParticleSimulation()
{
    wglib::Engine engine({2560, 1440}, "title");

    auto compute = engine.InitComputeLayer<wglib::compute::ParticleSimulationLayer>(
        10000, glm::vec2{2560, 1440}, 2, glm::vec4{0, 1, 1, 1}, glm::vec2{500, 500}, 100, 0.016, 500, 0.98, 2000, 50);

    wglib::render_layers::TextureRenderLayer textureRenderLayer{2560, 1440};

    engine.SetTargetFPS(120.0);

    std::function<void()> runIteration;

    runIteration = [&]() {
        engine.PushComputeLayer(compute, [&](std::optional<wgpu::Texture> res) {
            if (not res)
            {
                wglib::util::log("Failed to get results");
                return;
            }
            textureRenderLayer.setTexture(std::move(*res));
            runIteration();
        });
    };

    auto is_ready = true;

    engine.OnUpdate([&](auto) {
        if (is_ready)
        {
            is_ready = false;
            engine.PushComputeLayer(compute, [&](std::optional<wgpu::Texture> res) {
                textureRenderLayer.setTexture(*res);
                is_ready = true;
            });
        }

        engine.Draw(textureRenderLayer);
    });

    runIteration();

    engine.Start();
}

auto refactorTest()
{
    wglib::Engine engine{{500, 500}, "Game"};
    auto circleRenderLayer = engine.CreateRenderLayer<wglib::render_layers::CircleRenderLayer>(
        glm::vec2{250, 250}, 50.0f, glm::vec3{0.0f, 0.0f, 1.0f});
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        switch (std::stoi(argv[1]))
        {
        case 0:
            runParticleSimulation();
            break;
        case 1:
            refactorTest();
            break;
        case 2:
            runConwaysGameOfLife();
            break;
        default:
            runComputeAndDrawingExample();
        }
    }
    else
    {
        runConwaysGameOfLife();
    }
}

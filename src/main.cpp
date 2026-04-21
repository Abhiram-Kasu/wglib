#include <numbers>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include "GLFW/glfw3.h"
#include "lib/CoreEngine.hpp"
#include "lib/CoreRenderer.hpp"
#include "lib/render_layer/RenderLayer.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "lib/CoreUtil.hpp"

#include "lib/compute/ExampleLayers/ConwaysGameOfLife.hpp"
#include "lib/compute/ExampleLayers/ExampleLayer.hpp"
#include "lib/compute/ExampleLayers/ParticleSimulation.hpp"
#include "lib/render_layer/CircleRenderLayer.hpp"
#include "lib/render_layer/RectangleRenderLayer.hpp"
#include "lib/render_layer/TextureRenderLayer.hpp"
#include "lib/render_layer/TriangleRenderLayer.hpp"
#include "webgpu/webgpu_cpp.h"

auto runSimpleTriangleExample()
{
    auto engine = wglib::Engine(glm::vec2{1000, 1000}, "triangle");

    auto triangle = engine.CreateRenderLayer<wglib::render_layers::TriangleRenderLayer>(
        std::array<wglib::render_layers::Vertex, 3>{wglib::render_layers::Vertex{
                                                        .position = {1000.0f / 3, 1000 / 3 * 2},
                                                        .color = {1, 0, 0},
                                                    },
                                                    wglib::render_layers::Vertex{
                                                        .position = {1000.0f / 2, 1000 / 3},
                                                        .color = {0, 1, 0},
                                                    },
                                                    wglib::render_layers::Vertex{
                                                        .position = {1000.0f / 3 * 2, 1000 / 3 * 2},
                                                        .color = {0, 0, 1},
                                                    }});

    engine.OnUpdate([&](auto dt) { engine.Draw(triangle); });

    engine.Start();
}

auto runComputeAndDrawingExample()
{
    constexpr auto height = 1440uz;
    constexpr auto width = 1440uz;
    wglib::Engine engine(glm::vec2{height, width}, "title");

    auto rect1 = engine.CreateRenderLayer<wglib::render_layers::RectangleRenderLayer>(
        glm::vec2{10, 10}, glm::vec2{300, 300}, glm::vec3{0.0f, 1.0f, 0.0f});

    auto circle = engine.CreateRenderLayer<wglib::render_layers::CircleRenderLayer>(glm::vec2{250, 250}, 50.0f,
                                                                                    glm::vec3{0.0f, 0.0f, 1.0f});

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
        if (rect1->getPosition().x + rect1->getSize().x > width or rect1->getPosition().x < 0)
        {
            velocity.x *= -1;
        }
        if (rect1->getPosition().y + rect1->getSize().y > height or rect1->getPosition().y < 0)
        {
            velocity.y *= -1;
        }

        rect1->setPosition(rect1->getPosition() + velocity * static_cast<float>(s));

        engine.Draw(circle);
        auto radius = circle->getRadius() - circle->getRadius() * s;
        if (radius <= 10)
            radius = 100;
        circle->setRadius(radius);

        auto time = glfwGetTime();
        auto rotating_color = glm::vec3((sin(time * 1.0f) + 1.0f) * 0.5f, // Red channel
                                        (sin(time * 1.3f) + 1.0f) * 0.5f, // Green channel
                                        (sin(time * 1.7f) + 1.0f) * 0.5f  // Blue channel
        );
        circle->setColor(rotating_color);
    });

    engine.Start();
}

auto runConwaysGameOfLife()
{
    wglib::Engine engine({2560, 1440}, "title");

    auto compute = engine.InitComputeLayer<wglib::compute::ConwaysGameOfLifeComputeLayer>(glm::vec2{2560, 1440});
    auto textureRenderLayer = engine.CreateRenderLayer<wglib::render_layers::TextureRenderLayer>(2560, 1440);

    engine.SetTargetFPS(120.0);

    auto ready = true;
    engine.OnUpdate([&](auto) {
        if (ready)
        {
            ready = false;
            engine.PushComputeLayer(compute, [&](wgpu::Texture texture) {
                textureRenderLayer->setTexture(texture);
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

    auto textureRenderLayer = engine.CreateRenderLayer<wglib::render_layers::TextureRenderLayer>(2560, 1440);

    engine.SetTargetFPS(120.0);

    std::function<void()> runIteration;

    runIteration = [&]() {
        engine.PushComputeLayer(compute, [&](std::optional<wgpu::Texture> res) {
            if (not res)
            {
                wglib::util::log("Failed to get results");
                return;
            }
            textureRenderLayer->setTexture(std::move(*res));
            runIteration();
        });
    };

    auto is_ready = true;

    engine.OnUpdate([&](auto) {
        if (is_ready)
        {
            is_ready = false;
            engine.PushComputeLayer(compute, [&](std::optional<wgpu::Texture> res) {
                textureRenderLayer->setTexture(*res);
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
    engine.OnUpdate([&](auto dt) { engine.Draw(circleRenderLayer); });
    engine.Start();
}

auto interactionTest() -> void
{

    wglib::Engine engine{{500, 500}, "Game"};
    using Circle = wglib::Renderer::Ref<wglib::render_layers::CircleRenderLayer>;
    std::set<Circle> set;
    engine.OnUpdate([&](auto delta) {
        if (glfwGetMouseButton(engine.GetWindow(), GLFW_MOUSE_BUTTON_LEFT))
        {
            auto xPos = 0.0;
            auto yPos = 0.0;
            glfwGetCursorPos(engine.GetWindow(), &xPos, &yPos);

            set.insert(engine.CreateRenderLayer<wglib::render_layers::CircleRenderLayer>(glm::vec2{xPos, yPos}, 50.0f,
                                                                                         glm::vec3{0.0f, 0.0f, 1.0f}));
        }

        std::vector<Circle> toRemove{};
        toRemove.reserve(set.size());
        for (auto layer : set)
        {
            layer->setOrigin(layer->getOrigin() + glm::vec2{0, 1});

            if (layer->getOrigin().y + layer->getRadius() > 500)
            {
                toRemove.push_back(layer);
            }
            else
            {
                engine.Draw(layer);
            }
        }

        for (auto circle : toRemove)
        {
            set.erase(circle);
        }
    });

    engine.Start();
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
        case 3:
            runSimpleTriangleExample();
            break;
        case 4:
            interactionTest();
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

#ifdef __EMSCRIPTEN__
extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    int get_argc()
    {
        return 5;
    }
}
#endif

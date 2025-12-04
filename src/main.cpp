#include <iostream>
#include <ostream>

#include "lib/engine.hpp"


#include "lib/render_layer/rectangle_render_layer.hpp"


int main() {
    wglib::Engine engine({500, 500}, "title");

    engine.Draw<wglib::render_layers::RectangleRenderLayer>(
        glm::vec2{0, 0}, glm::vec2{100, 100}, glm::vec3{1.0f, 0.0f, 0.0f});
    engine.Draw<wglib::render_layers::RectangleRenderLayer>(
        glm::vec2{50, 50}, glm::vec2{100, 100}, glm::vec3{1.0f, 1.0f, 0.0f});
    engine.Draw<wglib::render_layers::RectangleRenderLayer>(
        glm::vec2{-500, 0}, glm::vec2{500, 500}, glm::vec3{1.0f, 1.0f, 1.0f});

    engine.OnUpdate([](float s) {
        std::cout << "Update function: " << s << std::endl;
    });

    engine.Start();
}
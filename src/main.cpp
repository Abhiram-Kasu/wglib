#include <__ostream/print.h>
#include <cmath>
#include <iostream>
#include <ostream>
#include <thread>

#include "lib/engine.hpp"

#include "lib/render_layer/CircleRenderLayer.hpp"
#include "lib/render_layer/rectangle_render_layer.hpp"

int main() {
  wglib::Engine engine({500, 500}, "title");

  // engine.Draw<wglib::render_layers::RectangleRenderLayer>(
  //     glm::vec2{0, 0}, glm::vec2{100, 100}, glm::vec3{1.0f, 0.0f, 0.0f});
  // engine.Draw<wglib::render_layers::RectangleRenderLayer>(
  //     glm::vec2{50, 50}, glm::vec2{100, 100}, glm::vec3{1.0f, 1.0f, 0.0f});
  // engine.Draw<wglib::render_layers::RectangleRenderLayer>(
  //     glm::vec2{-500, 0}, glm::vec2{500, 500}, glm::vec3{1.0f, 1.0f, 1.0f});
  wglib::render_layers::RectangleRenderLayer rect1(
      glm::vec2{0, 0}, glm::vec2{100, 100}, glm::vec3{0.0f, 1.0f, 0.0f});

  wglib::render_layers::CircleRenderLayer circle(glm::vec2{250, 250}, 50.0f,
                                                 glm::vec3{0.0f, 0.0f, 1.0f});

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
  });

  engine.Start();
}

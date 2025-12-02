//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace wglib::render_layers {
    struct Vertex {
        glm::vec2 position;
        glm::vec3 color;
    };
}
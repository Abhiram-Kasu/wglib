//
// Created by Abhiram Kasu on 12/10/25.
//

#include "CircleRenderLayer.hpp"

namespace wglib::render_layers
{
    CircleRenderLayer::CircleRenderLayer(glm::vec2 origin, float radius, glm::vec3 color)
        : m_origin(origin), m_radius(radius), m_color(color)
    {
    }
} // wglib
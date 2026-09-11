#include <wglib/CoreEngine.hpp>
#include <wglib/CoreUtil.hpp>
#include <wglib/compute/ComputeLayer.hpp>
#include <wglib/render_layer/CircleRenderLayer.hpp>
#include <wglib/render_layer/RectangleRenderLayer.hpp>

int main()
{
    return wglib::util::divCeil(9u, 4u) == 3u ? 0 : 1;
}

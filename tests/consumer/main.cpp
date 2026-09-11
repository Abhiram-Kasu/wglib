#include <wglib/CoreEngine.hpp>
#include <wglib/CoreUtil.hpp>

static void force_library_link(wglib::Engine *engine)
{
    delete engine;
}

int main()
{
    force_library_link(nullptr);
    return wglib::util::divCeil(9u, 4u) == 3u ? 0 : 1;
}

#include "renderpass_builder.hpp"

#include "windowed_renderpass.hpp"

namespace vke{

std::unique_ptr<Renderpass> make_simple_windowed_renderpass(Window* window, bool has_depth) {
    return std::make_unique<vke::WindowRenderPass>(window,has_depth);
}
} // namespace vke
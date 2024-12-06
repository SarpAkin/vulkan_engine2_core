#pragma once 

#include "renderpass.hpp"

namespace vke{

std::unique_ptr<Renderpass> make_simple_windowed_renderpass(Window* window, bool has_depth);
}
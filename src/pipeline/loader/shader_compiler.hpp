#pragma once

#include <vke/fwd.hpp>
#include <vulkan/vulkan.h>

#include <vector>

#include "pipeline_file.hpp"


namespace vke
{
    

struct CompiledShader{
public:
    std::vector<u32> spv;
    VkShaderStageFlagBits stage;
};

std::vector<CompiledShader> compile_shaders(PipelineDescription* description);
}
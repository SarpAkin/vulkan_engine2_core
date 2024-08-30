#include "pipeline.hpp"

#include "shader_reflection/pipeline_reflection.hpp" // IWYU pragma: keep
#include <vulkan/vulkan_core.h>

namespace vke {

Pipeline::Pipeline(VkPipeline pipeline, VkPipelineLayout layout, VkPipelineBindPoint bindpoint) {
    m_pipeline  = pipeline;
    m_layout    = layout;
    m_bindpoint = bindpoint;
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(device(), m_layout, nullptr);
}

} // namespace vke

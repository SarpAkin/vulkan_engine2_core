#include "pipeline_layout_builder.hpp"

#include "../vulkan_context.hpp"
#include "../vkutil.hpp"

namespace vke {

VkPipelineLayout PipelineLayoutBuilder::build() {
    VkPipelineLayoutCreateInfo info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = static_cast<uint32_t>(m_set_layouts.size()),
        .pSetLayouts            = m_set_layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(m_push_constants.size()),
        .pPushConstantRanges    = m_push_constants.data(),
    };

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(VulkanContext::get_context()->get_device(), &info, nullptr, &layout));
    return layout;
}

}
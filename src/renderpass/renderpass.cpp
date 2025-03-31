#include "renderpass.hpp"

#include <vulkan/vulkan_core.h>

#include <vke/util.hpp>

#include "../common.hpp"
#include "../fwd.hpp"

#include "../commandbuffer.hpp"
#include "../image.hpp"

namespace vke {

void Renderpass::set_states(CommandBuffer& cmd) {

    VkViewport view_port = {
        .x        = 0.f,
        .y        = 0.f,
        .width    = static_cast<float>(m_width),
        .height   = static_cast<float>(m_height),
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    vkCmdSetViewport(cmd.handle(), 0, 1, &view_port);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {m_width, m_height},
    };

    vkCmdSetScissor(cmd.handle(), 0, 1, &scissor);
}

void Renderpass::begin(CommandBuffer& cmd) {
    if (m_target_size) {
        if (m_target_size->width() != width() || m_target_size->height() != height()) {
            resize(cmd, m_target_size->width(), m_target_size->height());
        }
    }

    VkRenderPassBeginInfo rp_begin_info{
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = m_renderpass,
        .framebuffer = next_framebuffer(),
        .renderArea  = {
             .offset = {0, 0},
             .extent = {m_width, m_height},
        },
        .clearValueCount = static_cast<uint32_t>(m_clear_values.size()),
        .pClearValues    = m_clear_values.data(),
    };

    cmd.cmd_begin_renderpass(&rp_begin_info, m_is_external ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);

    if (!m_is_external) set_states(cmd);
}

void Renderpass::next_subpass(CommandBuffer& cmd) {
    cmd.cmd_next_subpass(m_is_external ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
}

void Renderpass::end(CommandBuffer& cmd) {
    cmd.cmd_end_renderpass();
}

void Renderpass::resize(CommandBuffer&, u32, u32) {
    assert(!"unimplemented Renderpass::resize function");
}

Renderpass::~Renderpass() {
    vkDestroyRenderPass(device(), m_renderpass, nullptr);
}
// void CommandBuffer::begin_renderpass(Renderpass* renderpass, VkSubpassContents contents) {
//     renderpass->begin(*this);
// }

VkRenderPass SubpassDetails::get_renderpass_handle() const { 
    return renderpass->handle(); 
}
} // namespace vke

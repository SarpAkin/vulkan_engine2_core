#include "renderpass_builder.hpp"

#include "windowed_renderpass.hpp"

#include "multi_pass_renderpass.hpp"

#include <vke/util.hpp>

namespace vke {

std::unique_ptr<Renderpass> make_simple_windowed_renderpass(Window* window, bool has_depth) {
    return std::make_unique<vke::WindowRenderPass>(window, has_depth);
}

using namespace impl;

u32 RenderPassBuilder::add_attachment(VkFormat format, std::optional<VkClearValue> clear_value, bool is_sampled) {
    m_attachment_infos.push_back(AttachmentInfo{
        .description = VkAttachmentDescription{
            .format         = format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = clear_value ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = is_sampled ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : (is_depth_format(format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL),
        },
        .is_sampled = is_sampled,
    });

    m_clear_values.push_back(clear_value.value_or(VkClearValue{}));

    return m_attachment_infos.size() - 1;
}

void RenderPassBuilder::add_subpass(const std::initializer_list<u32>& color_attachments, const std::optional<u32>& depth_attachment, const std::initializer_list<u32>& input_attachments) {
    m_subpass_info.push_back(SubpassInfo{
        .color_attachments = map_vec(color_attachments, [](u32 attachment_index) {
        return VkAttachmentReference{
            .attachment = attachment_index,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
    }),
        .depth_attachment  = map_optional(depth_attachment, [](u32 attachment_index) {
        return VkAttachmentReference{
             .attachment = attachment_index,
             .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    }),
        .input_attachments = map_vec(input_attachments, [](u32 attachment_index) {
        return VkAttachmentReference{
            .attachment = attachment_index,
            .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }),
    });

    // flag input attachments
    for (auto& input_attachment : input_attachments) {
        m_attachment_infos[input_attachment].is_input_attachment = true;
    }
}

VkRenderPass RenderPassBuilder::create_vk_renderpass() {
    auto ctx = vke::VulkanContext::get_context();

    auto subpasses = vke::map_vec2small_vec(m_subpass_info, [](const SubpassInfo& info) {
        return VkSubpassDescription{
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount    = static_cast<u32>(info.input_attachments.size()),
            .pInputAttachments       = info.input_attachments.data(),
            .colorAttachmentCount    = static_cast<u32>(info.color_attachments.size()),
            .pColorAttachments       = info.color_attachments.data(),
            .pDepthStencilAttachment = info.depth_attachment.has_value() ? &info.depth_attachment.value() : nullptr,
        };
    });

    auto attachments = vke::map_vec2small_vec(m_attachment_infos, [](const AttachmentInfo& info) { return info.description; });

    auto subpass_dependencies = create_subpass_dependencies();

    VkRenderPassCreateInfo render_pass_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<u32>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = static_cast<u32>(subpasses.size()),
        .pSubpasses      = subpasses.data(),
        .dependencyCount = static_cast<u32>(subpass_dependencies.size()),
        .pDependencies   = subpass_dependencies.data(),
    };

    VkRenderPass render_pass;
    VK_CHECK(vkCreateRenderPass(ctx->get_device(), &render_pass_info, nullptr, &render_pass));
    return render_pass;
}

std::unique_ptr<Renderpass> RenderPassBuilder::build(u32 width, u32 height) {
    return std::make_unique<MultiPassRenderPass>(this, width, height);
}

void merge_dependencies(std::vector<VkSubpassDependency>& dependencies) {
    std::vector<VkSubpassDependency> new_dependencies;

    for (auto& dep : dependencies) {
        for (auto& ndep : new_dependencies) {
            if (ndep.srcSubpass == dep.srcSubpass && ndep.dstSubpass == dep.dstSubpass) {
                ndep.srcStageMask |= dep.srcStageMask;
                ndep.dstStageMask |= dep.dstStageMask;
                ndep.srcAccessMask |= dep.srcAccessMask;
                ndep.dstAccessMask |= dep.dstAccessMask;
                ndep.dependencyFlags |= dep.dependencyFlags;
                goto end;
            }
        }

        new_dependencies.push_back(dep);
    end:;
    }

    dependencies = new_dependencies;
}

std::vector<VkSubpassDependency> RenderPassBuilder::create_subpass_dependencies() {
    std::vector<VkSubpassDependency> dependencies;

    auto attachement_uses = vke::SmallVec<std::optional<u32>,8>(m_attachment_infos.size());

    for (uint32_t i = 0; i < m_subpass_info.size(); ++i) {
        auto& subpass = m_subpass_info[i];

        for (auto& att : subpass.color_attachments) {
            auto& att_use = attachement_uses[att.attachment];
            if (att_use == std::nullopt) {
                att_use = i;
            } else {
                dependencies.push_back(VkSubpassDependency{
                    .srcSubpass      = static_cast<u32>(*att_use),
                    .dstSubpass      = i,
                    .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                });
            }
        }

        if (subpass.depth_attachment) {
            auto& att     = *subpass.depth_attachment;
            auto& att_use = attachement_uses[att.attachment];
            if (att_use == std::nullopt) {
                att_use = i;
            } else {
                dependencies.push_back(VkSubpassDependency{
                    .srcSubpass      = static_cast<u32>(*att_use),
                    .dstSubpass      = i,
                    .srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    .dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    .srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                });
            }
        }

        for (auto& input_att : subpass.input_attachments) {
            auto& att_use  = attachement_uses[input_att.attachment];
            auto& att_desc = m_attachment_infos[input_att.attachment];
            bool is_depth  = is_depth_format(att_desc.description.format);

            if (att_use == std::nullopt) {
                assert("input attachment must be a previously used as attachment" && 0);
            } else {
                dependencies.push_back(VkSubpassDependency{
                    .srcSubpass      = static_cast<u32>(*att_use),
                    .dstSubpass      = i,
                    .srcStageMask    = is_depth ? (VkPipelineStageFlags)(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT) : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .srcAccessMask   = is_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                });
            }
        }
    }

    merge_dependencies(dependencies);

    return dependencies;
}

} // namespace vke
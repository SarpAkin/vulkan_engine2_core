#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "common.hpp"
#include "fwd.hpp"
#include "util/slim_vec.hpp"

namespace vke {

struct PipelineRenderTargetDescription {
    vke::SmallVec<VkFormat> color_attachments;
    std::optional<VkFormat> depth_attachment;

    // these overwrite the values in pipeline builder so use responsibly
    std::optional<VkCompareOp> depth_compare_op;
    std::optional<VkCullModeFlagBits> cull_mode;
    VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;

    // these dynamic states should be used in all pipelines that target this
    vke::SmallVec<VkDynamicState> additional_dynamic_states;
};

class ISubpass {
public:
    virtual u32 get_subpass_index() const                 = 0;
    virtual VkRenderPass get_renderpass_handle() const    = 0;
    virtual u32 get_attachment_count() const              = 0;
    virtual vke::Renderpass* get_vke_renderpass() const   = 0;
    virtual std::unique_ptr<ISubpass> create_copy() const = 0;

    virtual bool is_renderpass() const { return true; }
    virtual PipelineRenderTargetDescription get_attachment_info() const = 0;

    virtual ~ISubpass() = default;
};

class SubpassDescription : public ISubpass {
public:
    SubpassDescription(VkRenderPass renderpass, u32 subpass_index, u32 attachment_count, vke::Renderpass* vke_renderpass)
        : subpass_index(subpass_index), renderpass(renderpass), attachment_count(attachment_count), vke_renderpass(vke_renderpass) {}

    u32 get_subpass_index() const override { return subpass_index; }
    VkRenderPass get_renderpass_handle() const override { return renderpass; }
    u32 get_attachment_count() const override { return attachment_count; }
    vke::Renderpass* get_vke_renderpass() const override { return vke_renderpass; }

    ~SubpassDescription() {}

private:
    u32 subpass_index, attachment_count;
    VkRenderPass renderpass;
    vke::Renderpass* vke_renderpass;
};

} // namespace vke
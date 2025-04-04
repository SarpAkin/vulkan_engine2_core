#pragma once 

#include "renderpass.hpp"

namespace vke{

std::unique_ptr<Renderpass> make_simple_windowed_renderpass(Window* window, bool has_depth);


namespace impl {
    struct AttachmentInfo {
        VkAttachmentDescription description;
        bool is_sampled;
        bool is_input_attachment = false;
        bool is_surface_attachment = false;
    };

    struct SubpassInfo {
        std::vector<VkAttachmentReference> color_attachments;
        std::optional<VkAttachmentReference> depth_attachment;
        std::vector<VkAttachmentReference> input_attachments;
    };
}

class MultiPassRenderPass;


class RenderPassBuilder {
    friend MultiPassRenderPass;
public:
    u32 add_attachment(VkFormat format, std::optional<VkClearValue> clear_value, bool is_sampled = false);
    void add_subpass(const std::initializer_list<u32>& color_attachments,const std::optional<u32>& depth_attachment,const std::initializer_list<u32>& input_attachments);

    std::unique_ptr<Renderpass> build(u32 width,u32 height);
private:
    std::vector<VkSubpassDependency> create_subpass_dependencies();
    VkRenderPass create_vk_renderpass();

    std::vector<impl::AttachmentInfo> m_attachment_infos;
    std::vector<VkClearValue> m_clear_values;
    std::vector<impl::SubpassInfo> m_subpass_info;
};
}
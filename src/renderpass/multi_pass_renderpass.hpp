#pragma once

#include <memory>
#include <vector>

#include "../image.hpp"

#include "../fwd.hpp"
#include "renderpass.hpp"

namespace vke {
class RenderPassBuilder;

namespace impl {
struct AttachmentInfo;
}

class MultiPassRenderPass : public Renderpass {
    friend RenderPassBuilder;

public:
    MultiPassRenderPass(RenderPassBuilder* builder, u32 width, u32 height);
    ~MultiPassRenderPass();

    IImageView* get_attachment_view(u32 index) override;
    bool has_depth(u32 subpass) override { return true; }

    void resize(CommandBuffer& cmd, u32 width, u32 height) override;

    void set_active_frame_buffer_instance(u32 i) override;

private:
    void create_attachments();
    void clear_sampled_attachments(CommandBuffer* cmd);
    void create_framebuffers();
    void destroy_framebuffers();

    VkFramebuffer next_framebuffer() override;

    void begin(CommandBuffer& cmd) override;

private:
    struct Attachment {
        vke::RCResource<vke::IImageView> image;
    };

    Window* m_window               = nullptr;
    bool m_has_surface_attachment  = false;
    u32 m_surface_attachment_index = UINT32_MAX;

    std::vector<RCResource<impl::Framebuffer>> m_framebuffers;
    std::vector<impl::AttachmentInfo> m_attachment_infos;

    std::vector<Attachment> m_attachments;
};
} // namespace vke

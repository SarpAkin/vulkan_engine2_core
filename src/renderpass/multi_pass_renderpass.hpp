#pragma once

#include <memory>
#include <vector>

#include "renderpass.hpp"
#include "../fwd.hpp"


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

private:
    void create_attachments();
    void create_framebuffers();
    void destroy_framebuffers();

    VkFramebuffer next_framebuffer() override;

    void begin(CommandBuffer& cmd) override;
private:
    struct Attachment{
        std::unique_ptr<Image> image;
    };

    Window* m_window = nullptr;
    bool m_has_surface_attachment = false;
    u32 m_surface_attachment_index = UINT32_MAX;

    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<impl::AttachmentInfo> m_attachment_infos;

    std::vector<Attachment> m_attachments;
};
} // namespace vke

#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../common.hpp"
#include "../fwd.hpp"
#include "../vk_resource.hpp"
#include "../window/surface.hpp"
#include "frame_buffer.hpp"

#include "../isubpass.hpp"

namespace vke {

class Renderpass;

struct SubpassDetails final : public ISubpass {
    std::vector<VkFormat> color_attachments;
    std::optional<VkFormat> depth_format;
    Renderpass* renderpass;
    u32 subpass_index;

public:
    virtual u32 get_subpass_index() const override { return subpass_index; }
    virtual VkRenderPass get_renderpass() const override;
    virtual u32 get_attachment_count() const override { return color_attachments.size(); };
    ~SubpassDetails() {};
    SubpassDetails()=default;
};

// an abstarct renderpass class
class Renderpass : public Resource, public IRenderTargetSize {
public: // getters
    u32 width() const override { return m_width; }
    u32 height() const override { return m_height; }
    inline VkRenderPass handle() const { return m_renderpass; }
    inline const SubpassDetails* get_subpass(usize subpass_index) const { return &m_subpasses[subpass_index]; }

    void set_target_size(IRenderTargetSize* target) { m_target_size = target; }

    virtual void resize(CommandBuffer& cmd, u32 width, u32 height);

public: // methods
    virtual void set_states(CommandBuffer& cmd);

    virtual void begin(CommandBuffer& cmd);
    virtual void next_subpass(CommandBuffer& cmd);
    virtual void end(CommandBuffer& cmd);
    virtual void set_external(bool is_external) { m_is_external = is_external; }
    virtual bool has_depth(u32 subpass) { return false; }

    Renderpass() {}
    ~Renderpass();

protected:
    virtual VkFramebuffer next_framebuffer() = 0;

protected:
    bool m_is_external = false;
    u32 m_width, m_height;
    VkRenderPass m_renderpass = nullptr; // should be destroyed by this class, created by child class.
    IRenderTargetSize* m_target_size = nullptr;
    std::vector<VkClearValue> m_clear_values;
    std::vector<SubpassDetails> m_subpasses; // shouldn't be modified after creation. especially should't be resized since pointers to elements might be created.
};

} // namespace vke
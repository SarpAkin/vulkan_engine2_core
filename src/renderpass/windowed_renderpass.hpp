#pragma once

#include "renderpass.hpp"

namespace vke{

class WindowRenderPass : public Renderpass {
public:
    WindowRenderPass(Window* window, bool include_depth_buffer = true);
    ~WindowRenderPass();

    void begin(CommandBuffer& cmd) override;
    void resize(CommandBuffer& cmd,u32 width, u32 height) override;

    bool has_depth(u32 subpass) override { return true; }

private:
    void init_renderpass();
    void create_framebuffers();
    void destroy_framebuffers();
    void add_framebuffers_as_dependency(vke::CommandBuffer& cmd);

    VkFramebuffer next_framebuffer() override;

private:
    Window* m_window = nullptr;
    std::vector<RCResource<impl::Framebuffer>> m_framebuffers;
    std::unique_ptr<vke::Image> m_depth;
    std::vector<VkImageView> m_swapchain_image_views;
};

}
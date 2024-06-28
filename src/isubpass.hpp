#pragma once

#include "common.hpp"
#include <vulkan/vulkan.h>

namespace vke {

class ISubpass {
public:
    virtual u32 get_subpass_index()const        = 0;
    virtual VkRenderPass get_renderpass()const = 0;
    virtual u32 get_attachment_count()const    = 0;
};

class SubpassDescription : public ISubpass {
public:
    u32 subpass_index, attachment_count;
    VkRenderPass renderpass;

    u32 get_subpass_index() const override {
        return subpass_index;
    }

    VkRenderPass get_renderpass() const override {
        return renderpass;
    }

    u32 get_attachment_count() const override {
        return attachment_count;
    }
};

} // namespace vke
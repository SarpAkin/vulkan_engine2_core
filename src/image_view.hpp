#pragma once

#include "fwd.hpp"
#include "image.hpp"

namespace vke {

class ImageView : public IImageView {
public:
    ImageView(Image* image, const VkImageViewCreateInfo& c_info);
    ~ImageView();

    Image* vke_image() override { return m_vke_image; }
    const Image* vke_image() const override { return m_vke_image; }

    VkImageView view() const override { return m_view; }

    u32 layer_count() const override { return m_subresource_range.layerCount; }
    u32 miplevel_count() const override { return m_subresource_range.levelCount; }
    u32 base_layer() const override { return m_subresource_range.baseArrayLayer; }
    u32 base_miplevel() const override { return m_subresource_range.baseMipLevel; }

    VkImageViewType view_type() const override { return m_view_type; }

    VkImageSubresourceRange get_subresource_range() const override { return m_subresource_range; }

    virtual u32 width() const override{return m_extends.width;}
    virtual u32 height()const override{return m_extends.height;}

private:
    VkImageView m_view = VK_NULL_HANDLE;
    Image* m_vke_image;
    VkImageSubresourceRange m_subresource_range;
    VkImageViewType m_view_type;
    VkExtent2D m_extends;
};

} // namespace vke
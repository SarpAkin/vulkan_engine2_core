#include "image_view.hpp"

namespace vke {

constexpr VkExtent2D calculate_mip_extent(const VkExtent2D& base_extent, uint32_t mip_level) {
    auto mip_size = [](uint32_t size, uint32_t level) -> uint32_t {
        return std::max(1u, (size + (1u << level) - 1) >> level);
    };

    return {
        mip_size(base_extent.width, mip_level),
        mip_size(base_extent.height, mip_level),
    };
}

ImageView::ImageView(Image* image, const VkImageViewCreateInfo& c_info) {
    m_vke_image = image;

    m_subresource_range = c_info.subresourceRange;
    m_view_type         = c_info.viewType;

    m_extends =  calculate_mip_extent(image->extend(), c_info.subresourceRange.baseMipLevel);

    vkCreateImageView(device(), &c_info, nullptr, &m_view);

#ifndef NDEBUG
    m_vke_image->m_image_view_counter++;
#endif // !NDEBUG
}

ImageView::~ImageView() {
    vkDestroyImageView(device(), m_view, nullptr);

#ifndef NDEBUG
    m_vke_image->m_image_view_counter--;
#endif // !NDEBUG
}
} // namespace vke

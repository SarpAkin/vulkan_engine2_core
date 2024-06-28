#include "image_view.hpp"

namespace vke {

ImageView::ImageView(Image* image, const VkImageViewCreateInfo& c_info) {
    m_vke_image = image;

    m_subresource_range = c_info.subresourceRange;
    m_view_type         = c_info.viewType;

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

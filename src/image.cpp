#include "image.hpp"

#include <atomic>
#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_context.hpp"


#include "commandbuffer.hpp"

#include "util/util.hpp"
#include "buffer.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"

#include "image_view.hpp"

namespace vke {

Image::Image(const ImageArgs& args) {
    m_width       = args.width;
    m_height      = args.height;
    m_format      = args.format;
    m_num_layers  = args.layers;
    m_num_mipmaps = args.mip_levels;
    m_aspects     = is_depth_format(args.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageCreateInfo ic_info {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = args.create_flags,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = args.format,
        .extent        = {.width = args.width, .height = args.height, .depth = 1},
        .mipLevels     = args.mip_levels,
        .arrayLayers   = args.layers,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = args.host_visible ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
        .usage         = args.usage_flags,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo dimg_allocinfo = {
        .flags = args.host_visible ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0u,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };


    auto gpu_alloc = VulkanContext::get_context()->gpu_allocator();

    VK_CHECK(vmaCreateImage(gpu_alloc, &ic_info, &dimg_allocinfo, &m_image, &m_allocation, nullptr));
    vmaSetAllocationName(gpu_alloc, m_allocation, "image");

    // VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT

    m_view_type = args.layers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;

    VkImageViewCreateInfo ivc_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = m_image,
        .viewType = m_view_type,
        .format   = args.format,

        .subresourceRange = {
            .aspectMask     = aspects(),
            .baseMipLevel   = 0,
            .levelCount     = args.mip_levels,
            .baseArrayLayer = 0,
            .layerCount     = args.layers,
        },
    };

    VK_CHECK(vkCreateImageView(device(), &ivc_info, nullptr, &m_view));

    if (args.host_visible) {
        VK_CHECK(vmaMapMemory(gpu_alloc, m_allocation, &m_mapped_data));
    }

}


Image::~Image() {
#ifndef NDEBUG

    if (m_image_view_counter != 0) {
        fprintf(stderr, "alive image views exist to the image being destroyed. counter: %d", (int)m_image_view_counter);
    }
#endif

    auto gpu_alloc = VulkanContext::get_context()->gpu_allocator();

    if (m_mapped_data) vmaUnmapMemory(gpu_alloc, m_allocation);

    vkDestroyImageView(device(), m_view, nullptr);
    vmaDestroyImage(gpu_alloc, m_image, m_allocation);
}

std::unique_ptr<Image> Image::buffer_to_image(CommandBuffer& cmd, IBufferSpan* buffer, const ImageArgs& _args, VkImageLayout final_layout) {
    ImageArgs args = _args;
    args.usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    auto image = std::make_unique<Image>(args);

    image->copy_from_buffer(cmd,
        CopyFromBufferArgs{
            .buffer       = buffer,
            .final_layout = final_layout,
        });

    return image;
}

void Image::copy_from_buffer(CommandBuffer& cmd, std::span<const CopyFromBufferArgs> vargs) {
    auto queue = VulkanContext::get_context()->get_graphics_queue_family();

    auto barriers = MAP_VEC_ALLOCA(vargs, [&](const CopyFromBufferArgs& args) {
        return VkImageMemoryBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = args.initial_layout,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = queue,
            .dstQueueFamilyIndex = queue,
            .image               = handle(),
            .subresourceRange    = VkImageSubresourceRange{
                   .aspectMask     = aspects(),
                   .baseMipLevel   = 0,
                   .levelCount     = 1,
                   .baseArrayLayer = args.layer,
                   .layerCount     = args.layer_count,
            },
        };
    });

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .image_memory_barriers = barriers,
    });

    for (auto& args : vargs) {
        VkBufferImageCopy copy_region = {
            .bufferOffset      = args.buffer->byte_offset(),
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = VkImageSubresourceLayers{
                 .aspectMask     = aspects(),
                 .mipLevel       = 0,
                 .baseArrayLayer = 0,
                 .layerCount     = 1,
            },
            .imageExtent = VkExtent3D{
                .width  = static_cast<u32>(width()),
                .height = static_cast<u32>(height()),
                .depth  = 1,
            },
        };

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd.handle(), args.buffer->handle(), handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    }

    // reuse barriers array
    for (int i = 0; i < vargs.size(); i++) {
        auto& barrier = barriers[i];
        auto& args    = vargs[i];

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = args.final_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .image_memory_barriers = barriers,
    });
}

std::unique_ptr<IImageView> Image::create_subview(const SubViewArgs& args) {
    VkImageViewCreateInfo ivc_info = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = m_image,
        .viewType         = args.view_type,
        .format           = format(),
        .subresourceRange = {
            .aspectMask     = aspects(),
            .baseMipLevel   = args.base_miplevel,
            .levelCount     = args.miplevel_count == UINT32_MAX ? miplevel_count() - args.base_miplevel : args.base_miplevel,
            .baseArrayLayer = args.base_layer,
            .layerCount     = args.layer_count,
        },
    };

    return std::make_unique<ImageView>(this, ivc_info);
}

VkFormat IImageView::format() { return vke_image()->format(); }
u32 IImageView::height() {return vke_image()->height(); }
u32 IImageView::width() {return vke_image()->width(); }
} // namespace vke
#pragma once

#include <atomic>
#include <memory>
#include <span>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {

struct ImageArgs {
    VkFormat format;
    VkImageUsageFlags usage_flags;
    VkImageCreateFlags create_flags;
    u32 width;
    u32 height;
    u32 layers        = 1; // 1
    u32 mip_levels    = 1; // 1
    bool host_visible = false;
};

struct CopyFromBufferArgs {
    IBufferSpan* buffer;
    u32 layer       = 0; // 0 by default
    u32 layer_count = 1; // 1 by default
    // VK_IMAGE_LAYOUT_UNDEFINED by default
    VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL by default
    VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
};

struct SubViewArgs {
    u32 base_layer;
    u32 layer_count    = 1;
    u32 base_miplevel  = 0;
    u32 miplevel_count = UINT_MAX; // by default same as images miplevel count
    VkImageViewType view_type;
};

class IImageView {
public:
    virtual VkImageView view() const = 0;
    virtual Image* vke_image()       = 0;
    virtual VkFormat format();

    virtual u32 base_layer() const     = 0;
    virtual u32 layer_count() const    = 0;
    virtual u32 base_miplevel() const  = 0;
    virtual u32 miplevel_count() const = 0;

    virtual VkImageViewType view_type() const = 0;

    virtual VkSampler get_default_sampler() const { return VK_NULL_HANDLE; }

    virtual u32 width();
    virtual u32 height();

    virtual ~IImageView() {};
};

class ImageView;

class Image : public Resource, public IImageView {
    friend ImageView;

public:
    Image(const ImageArgs& args);
    ~Image();

public: // getters
    VkImage handle() const { return m_image; }
    VkImageView view() const override { return m_view; }
    VkFormat format() const { return m_format; }
    VkImageAspectFlags aspects() { return m_aspects; }

    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    u32 layer_count() const override { return m_num_layers; }
    u32 miplevel_count() const override { return m_num_mipmaps; }

    VkImageViewType view_type() const override {
        return m_view_type;
    }

    template <typename T>
    T* mapped_data_ptr() {
        return reinterpret_cast<T*>(m_mapped_data);
    }

public: // util
    std::unique_ptr<IImageView> create_subview(const SubViewArgs& arsg);

    void copy_from_buffer(CommandBuffer& cmd, const CopyFromBufferArgs& args) { copy_from_buffer(cmd, std::span(&args, 1)); }
    void copy_from_buffer(CommandBuffer& cmd, std::span<const CopyFromBufferArgs> args);

    // blocking
    void save_as_png(const char* path);

public: // static methods
    static std::unique_ptr<Image> image_from_bytes(CommandBuffer& cmd, const std::span<const u8>& bytes, const ImageArgs& args, VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    static std::unique_ptr<Image> buffer_to_image(CommandBuffer& cmd, IBufferSpan* buffer, const ImageArgs& args, VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    static std::unique_ptr<Image> load_png(CommandBuffer& cmd, const char* path, u32 mip_levels = 1); // image_load.cpp

private: // hide unnecessary methods from interface IImageView
    Image* vke_image() override { return this; }
    u32 base_layer() const override { return 0; }
    u32 base_miplevel() const override { return 0; }

private: // private fields
    VkImage m_image;
    VkImageView m_view;
    VmaAllocation m_allocation;
    void* m_mapped_data = nullptr;

    VkFormat m_format;
    VkImageAspectFlags m_aspects;
    VkImageViewType m_view_type;

    std::atomic<i32> m_image_view_counter = 0;

    u32 m_width, m_height, m_num_layers, m_num_mipmaps;
};

} // namespace vke
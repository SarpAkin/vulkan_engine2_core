#include "image.hpp"
#include "vulkan_context.hpp"
#include <cstring>
#include <type_traits>


#include <stb_image.h>
#include <stb_image_write.h>

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <vke/util.hpp>

#include "buffer.hpp"
#include "commandbuffer.hpp"
#include "vk_resource.hpp"

namespace vke {

std::unique_ptr<Image> Image::load_png(CommandBuffer& cmd, const char* path, u32 mip_levels) {

    int tex_width, tex_height, tex_channels;

    stbi_uc* pixels = stbi_load(path, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels) {
        THROW_ERROR("failed to load file: %s", path);
    }

    size_t buf_size = tex_width * tex_height * 4;

    // might leak pixels if an error is thrown
    RCResource<Buffer> stencil = std::make_unique<Buffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, buf_size, true);
    memcpy(stencil->mapped_data<char>().data(), pixels, buf_size);

    stbi_image_free(pixels);

    auto image = Image::buffer_to_image(cmd, stencil.get(),
        ImageArgs{
            .format      = VK_FORMAT_R8G8B8A8_SRGB,
            .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
            .width       = static_cast<u32>(tex_width),
            .height      = static_cast<u32>(tex_height),
            .layers      = 1,
        });

    cmd.add_execution_dependency(stencil->get_reference());

    return image;
}

} // namespace vke
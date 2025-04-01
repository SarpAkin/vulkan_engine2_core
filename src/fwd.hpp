#pragma once

namespace vke {

class ISubpass;

class VulkanContext;
class Buffer;
class IBuffer;
class GrowableBuffer;
class IBufferSpan;
class BufferSpan;

class DescriptorPool;

class CommandBuffer;
class Pipeline;
class Image;
class ImageView;
class IImageView;
class Fence;

class ArenaAllocator;

class GPipelineBuilder;
class CPipelineBuilder;

class Semaphore;
class Fence;

class IPipelineLoader;
class IPipeline;

class Window;
class Renderpass;
} // namespace vke

// vulkan-hpp fwds
namespace vk {
class Instance;
class PhysicalDevice;
class Device;
class Image;
class Buffer;
class CommandBuffer;
class CommandPool;

namespace detail {
class DispatchLoaderDynamic;
}
} // namespace vk
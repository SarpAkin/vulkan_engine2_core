#pragma once

#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <vke/fwd.hpp>

#include "../../util/arena_alloc.hpp"
#include "../../vk_resource.hpp"

#include "pipeline_layout_builder.hpp"

struct SpvReflectBlockVariable;

namespace vke {

class PipelineReflection;

class PipelineLayoutBuilder;

class PipelineBuilderBase : protected Resource {
public:
    PipelineBuilderBase();
    ~PipelineBuilderBase();

    // only works when compiling glsl
    void set_shader_compile_defines(std::span<const std::pair<std::string, std::string>> defines) { m_defines = defines; }

    // 0 is for auto
    void add_shader_stage(const u32* spirv_code, usize spirv_len, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0, std::string_view filename = "");
    void add_shader_stage(std::span<const u8> span, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0, std::string_view filename = "") {
        assert(span.size() % 4 == 0);
        add_shader_stage(reinterpret_cast<const u32*>(span.data()), span.size() / 4, stage, filename);
    }
    void add_shader_stage(std::span<const u32> span, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0, std::string_view filename = "") { add_shader_stage(span.data(), span.size(), stage, filename); };
    void add_shader_stage(std::string_view spirv_path);
    void set_layout_builder(PipelineLayoutBuilder* builder) { m_layout_builder = builder; }
    void set_pipeline_cache(VkPipelineCache cache) { m_pipeline_cache = cache; }

    void set_descriptor_set_layout(int set_index, VkDescriptorSetLayout layout);

    const PipelineReflection* get_reflection() const { return m_reflection.get(); };

protected:
    std::span<const std::pair<std::string, std::string>> m_defines;
    std::unique_ptr<PipelineReflection> m_reflection;

    struct ShaderDetails {
        const u32* spirv_code;
        u32 spirv_len;
        VkShaderStageFlagBits stage;
        VkShaderModule module;
        std::string file_path;
    };

    std::vector<ShaderDetails> m_shader_details;
    PipelineLayoutBuilder* m_layout_builder;
    std::unique_ptr<PipelineLayoutBuilder> m_owned_builder;
    VkPipelineCache m_pipeline_cache = VK_NULL_HANDLE;
    VkShaderStageFlags m_shader_stages;
    ArenaAllocator m_arena;
};

class VertexInputDescriptionBuilder;

class GPipelineBuilder : public PipelineBuilderBase {
public:
    GPipelineBuilder();

    void set_renderpass(ISubpass* subpass);
    void set_renderpass(VkRenderPass renderpass, u32 subpass_index, u32 attachment_count);
    void set_subpass_name(const std::string& name) { m_subpass_name = name; }

    void set_topology(VkPrimitiveTopology topology);                                                                           // Set to triangle list by default
    void set_rasterization(VkPolygonMode polygon_mode, VkCullModeFlagBits cull_mode);                                          // Set to Triangle Fill & No Cull
    void set_depth_testing(bool depth_testing, bool depth_write = true, VkCompareOp compare_op = VK_COMPARE_OP_LESS_OR_EQUAL); // Defaults to true if renderpass has depth buffer else false
    inline void set_vertex_input(const VertexInputDescriptionBuilder* builder) { m_input_description_builder = builder; };

    std::unique_ptr<Pipeline> build();

private:
    void set_opaque_color_blend();

    const VertexInputDescriptionBuilder* m_input_description_builder = nullptr;
    ISubpass* m_isubpass                                             = nullptr;
    VkRenderPass m_renderpass                                        = VK_NULL_HANDLE;
    u32 m_subpass_index                                              = 0;
    u32 m_attachment_count                                           = 0;
    bool default_depth                                               = true;

    VkPipelineDepthStencilStateCreateInfo m_depth_stencil                      = {};
    VkPipelineVertexInputStateCreateInfo m_vertex_input_info                   = {};
    VkPipelineInputAssemblyStateCreateInfo m_input_assembly                    = {};
    VkPipelineRasterizationStateCreateInfo m_rasterizer                        = {};
    VkPipelineMultisampleStateCreateInfo m_multisampling                       = {};
    std::vector<VkPipelineColorBlendAttachmentState> m_color_blend_attachments = {};

    std::string m_subpass_name;
};

class CPipelineBuilder : public PipelineBuilderBase {
public:
    CPipelineBuilder();

    std::unique_ptr<Pipeline> build();
};

} // namespace vke

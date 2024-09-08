#include "pipeline_builder.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <vulkan/vulkan_core.h>

#include <spirv_reflect.h>

#include "../pipeline.hpp"
#include "../shader_reflection/pipeline_reflection.hpp"
#include "../vkutil.hpp"
#include "vertex_input_builder.hpp"

#include "../util/util.hpp"


namespace vke {

void PipelineBuilderBase::set_descriptor_set_layout(int set_index, VkDescriptorSetLayout layout) {
    m_reflection->set_descriptor_layout(set_index, layout);
}


PipelineBuilderBase::~PipelineBuilderBase() {
    for (auto& shader : m_shader_details) {
        vkDestroyShaderModule(device(), shader.module, nullptr);
    }
}

PipelineBuilderBase::PipelineBuilderBase() {
    m_reflection = std::make_unique<PipelineReflection>();
}

void PipelineBuilderBase::add_shader_stage(const u32* spirv_code, usize spirv_len, VkShaderStageFlagBits stage, std::string_view filename) {

    VkShaderModule module = VK_NULL_HANDLE;

    auto reflected_stage = m_reflection->add_shader_stage(std::span(spirv_code, spirv_len));

    if (stage == 0) {
        stage = reflected_stage;
    }

    VkShaderModuleCreateInfo c_info{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv_len * 4, // convert to byte size
        .pCode    = spirv_code,
    };

    VK_CHECK(vkCreateShaderModule(device(), &c_info, nullptr, &module));

    m_shader_details.push_back(ShaderDetails{
        .spirv_code = spirv_code,
        .spirv_len  = static_cast<u32>(spirv_len),
        .stage      = stage,
        .module     = module,
        .file_path  = std::string(filename),
    });
}

void GPipelineBuilder::set_topology(VkPrimitiveTopology topology) {
    m_input_assembly = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
    };
}

void GPipelineBuilder::set_rasterization(VkPolygonMode polygon_mode, VkCullModeFlagBits cull_mode) {
    m_rasterizer = {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = polygon_mode,
        .cullMode    = static_cast<VkCullModeFlags>(cull_mode),
        .frontFace   = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth   = 1.f,
    };
}

void GPipelineBuilder::set_depth_testing(bool depth_testing) {
    m_depth_stencil = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = depth_testing,
        .depthWriteEnable = depth_testing,
        .depthCompareOp   = depth_testing ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS,
    };

    default_depth = false;
}

GPipelineBuilder::GPipelineBuilder() {
    m_multisampling = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading     = 1.0f,
    };

    set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);
    set_depth_testing(false);
}

std::unique_ptr<Pipeline> GPipelineBuilder::build() {
    assert(m_renderpass);

    if (default_depth) {
        // set_depth_testing(m_renderpass->has_depth(m_subpass_index));
        set_depth_testing(false);
    }

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    // auto subpass = m_renderpass->get_subpass(m_subpass_index);

    m_color_blend_attachments.resize(m_attachment_count,
        VkPipelineColorBlendAttachmentState{
            .blendEnable    = false,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        });

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<u32>(m_color_blend_attachments.size()),
        .pAttachments    = m_color_blend_attachments.data(),
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<u32>(ARRAY_LEN(dynamic_states)),
        .pDynamicStates    = dynamic_states,
    };

    // must be called before creating shader stages since it can create shader modules if their stage is left to default
    auto layouts = m_reflection->build_pipeline_layout();

    bool is_mesh_shader = false;

    auto shader_stages = MAP_VEC_ALLOCA(m_shader_details, [&](const ShaderDetails& shader_detail) {
        if (shader_detail.stage == VK_SHADER_STAGE_MESH_BIT_EXT) is_mesh_shader = true;

        return VkPipelineShaderStageCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = shader_detail.stage,
            .module = shader_detail.module,
            .pName  = "main",
        };
    });

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
    };
    if (m_input_description_builder) {
        vertex_input_info = m_input_description_builder->get_info();
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = static_cast<uint32_t>(shader_stages.size()),
        .pStages             = shader_stages.data(),
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &m_input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &m_rasterizer,
        .pMultisampleState   = &m_multisampling,
        .pDepthStencilState  = &m_depth_stencil,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = &dynamic_state_info,
        .layout              = layouts.layout,
        .renderPass          = m_renderpass,
        .subpass             = m_subpass_index,
        .basePipelineHandle  = VK_NULL_HANDLE,
    };

    if (is_mesh_shader) {
        // printf("mesh shader compiled!!!\n");
    }

    VkPipeline pipeline;
    auto result = vkCreateGraphicsPipelines(device(), m_pipeline_cache, 1, &pipeline_info, nullptr, &pipeline);

    if (result != VK_SUCCESS) {
        THROW_ERROR("failed to build pipeline %s",vke::vk_result_string(result).c_str());
    }

    if (is_mesh_shader) {
        printf("mesh shader compiled!!!\n");
    }

    auto vke_pipeline                 = std::make_unique<Pipeline>(pipeline, layouts.layout, VK_PIPELINE_BIND_POINT_GRAPHICS);
    vke_pipeline->m_data.dset_layouts = std::move(layouts.dset_layouts);
    vke_pipeline->m_data.push_stages  = layouts.push_stages;
    vke_pipeline->m_reflection        = std::move(m_reflection);
    return vke_pipeline;
}

CPipelineBuilder::CPipelineBuilder() {}

std::unique_ptr<Pipeline> CPipelineBuilder::build() {
    assert(m_shader_details.size() == 1 && "compute pipeline must have exactly one shader module");

    auto layout_details = m_reflection->build_pipeline_layout();

    VkComputePipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = VkPipelineShaderStageCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = m_shader_details[0].module,
            .pName  = "main",
        },
        .layout = layout_details.layout,
    };

    VkPipeline vk_pipeline;
    vkCreateComputePipelines(device(), m_pipeline_cache, 1, &create_info, nullptr, &vk_pipeline);

    auto vke_pipeline                 = std::make_unique<Pipeline>(vk_pipeline, layout_details.layout, VK_PIPELINE_BIND_POINT_COMPUTE);
    vke_pipeline->m_data.dset_layouts = std::move(layout_details.dset_layouts);
    vke_pipeline->m_data.push_stages  = layout_details.push_stages;
    vke_pipeline->m_reflection        = std::move(m_reflection);

    return vke_pipeline;
}

void PipelineBuilderBase::add_shader_stage(std::string_view spirv_path) {
    std::filesystem::path p = spirv_path;

    if (p.extension() != ".spv") {
        fprintf(stderr, "failed to load shader. path: %s\n", p.c_str());



        // ShaderCompileOptions options{
        //     .defines = m_defines,
        // };

        // auto binary = compile_glsl_file(&m_arena, spirv_path.begin(), &options);
        // this->add_shader_stage(binary, VkShaderStageFlagBits(0), spirv_path);

    } else {
        auto binary = read_file_binary(&m_arena, spirv_path.data());
        this->add_shader_stage(binary, VkShaderStageFlagBits(0), spirv_path);
    }
}

void GPipelineBuilder::set_renderpass(ISubpass* subpass) {
    set_renderpass(subpass->get_renderpass(), subpass->get_subpass_index(), subpass->get_attachment_count());
}
void GPipelineBuilder::set_renderpass(VkRenderPass renderpass, u32 subpass_index, u32 attachment_count) {
    m_renderpass       = renderpass;
    m_subpass_index    = subpass_index;
    m_attachment_count = attachment_count;
};
} // namespace vke
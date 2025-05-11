#pragma once

#include <span>
#include <vector>
#include <vke/fwd.hpp>
#include <vulkan/vulkan.h>

#include "../vk_resource.hpp"

namespace vke {

class GPUTimer : Resource {
public:
    struct RawTimeStamps {
        std::string label;
        u64 raw_time;
    };

public:
    GPUTimer(bool is_enabled = true, u32 max_queries = 512);
    ~GPUTimer() {}

    void timestamp(vke::CommandBuffer& cmd, std::string_view label, VkPipelineStageFlagBits stage);

    void reset(vke::CommandBuffer& cmd);

    void begin_frame(vke::CommandBuffer& cmd);
    void end_frame(vke::CommandBuffer& cmd);

    bool is_enabled() const { return m_enabled; }
    void set_enabled(bool enabled) { m_enabled = enabled; }

    void query_results();

    void sort_results();

    std::span<const RawTimeStamps> get_labels() const { return m_query_labels; }

    double get_delta_time_in_miliseconds(u32 index_a, u32 index_b) const;

private:
private:
    VkQueryPool m_timing_pool = VK_NULL_HANDLE;
    std::vector<RawTimeStamps> m_query_labels;
    u32 m_max_queries = 512, m_query_counter = 0;
    bool m_enabled              = true;
    bool m_is_resetted_at_start = false;
};

} // namespace vke
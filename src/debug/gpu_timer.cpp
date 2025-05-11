#include "gpu_timer.hpp"

#include <vke/util.hpp>

#include "../commandbuffer.hpp"
#include <vulkan/vulkan.hpp>

namespace vke {

GPUTimer::GPUTimer(bool is_enabled, u32 max_queries) {
    m_enabled     = is_enabled;
    m_max_queries = max_queries;

    VkQueryPoolCreateInfo info{
        .sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType  = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = m_max_queries,
    };

    dt().vkCreateQueryPool(device(), &info, nullptr, &m_timing_pool);
}

void GPUTimer::timestamp(vke::CommandBuffer& cmd, std::string_view label, VkPipelineStageFlagBits stage) {
    if (!is_enabled()) return;

    u32 index = m_query_counter++;
    assert(index < m_max_queries);

    dt().vkCmdWriteTimestamp(cmd.handle(), stage, m_timing_pool, index);

    m_query_labels.push_back({
        .label = std::string(label),
    });
}

void GPUTimer::query_results() {
    assert(m_query_counter == m_query_labels.size());

    if (m_query_counter == 0) return;

    VK_CHECK(dt().vkGetQueryPoolResults(
        device(), m_timing_pool, 0, m_query_counter,
        sizeof(RawTimeStamps) * m_query_counter, &m_query_labels[0].raw_time, sizeof(RawTimeStamps),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT //
        ));
}

void GPUTimer::reset(vke::CommandBuffer& cmd) {
    dt().vkCmdResetQueryPool(cmd.handle(), m_timing_pool, 0, m_is_resetted_at_start ? m_query_counter : m_max_queries);

    m_is_resetted_at_start = true;

    m_query_counter = 0;
    m_query_labels.clear();
}

void GPUTimer::begin_frame(vke::CommandBuffer& cmd) {
    assert(m_query_counter == 0 && "reset() must be called before begining a new frame");

    timestamp(cmd, "FRAME_BEGIN", VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}
void GPUTimer::end_frame(vke::CommandBuffer& cmd) { timestamp(cmd, "FRAME_END", VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT); }

double GPUTimer::get_delta_time_in_miliseconds(u32 index_a, u32 index_b) const {
    double diff = m_query_labels[index_b].raw_time >= m_query_labels[index_a].raw_time
                      ? +static_cast<double>(m_query_labels[index_b].raw_time - m_query_labels[index_a].raw_time)
                      : -static_cast<double>(m_query_labels[index_a].raw_time - m_query_labels[index_b].raw_time);

    return diff / 1E6;
}
void GPUTimer::sort_results() {
    std::sort(m_query_labels.begin(), m_query_labels.end(), [&](const RawTimeStamps& a, const RawTimeStamps& b) {
        return a.raw_time < b.raw_time;
    });
}
} // namespace vke
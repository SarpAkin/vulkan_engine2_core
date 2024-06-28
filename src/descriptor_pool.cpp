#include "descriptor_pool.hpp"

#include "vkutil.hpp"

#include "util/util.hpp"

namespace vke {

VkDescriptorSet DescriptorPool::allocate_set(VkDescriptorSetLayout layout) {

    if (!m_current_pool) next_pool();

    VkDescriptorSetAllocateInfo info{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = m_current_pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
    };

    VkDescriptorSet set;

    auto result = vkAllocateDescriptorSets(device(), &info, &set);
    if (result == VK_SUCCESS) {
        return set;
    } else if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY) {
        next_pool();
        return allocate_set(layout);
    } else {
        VK_CHECK(result);
    }
    return nullptr;
}

void DescriptorPool::next_pool() {

    if (m_current_pool) m_used_pools.push_back(m_current_pool);

    m_current_pool = nullptr;

    if (m_free_pools.size()) {
        m_current_pool = m_free_pools.back();
        m_free_pools.pop_back();
    } else {
        auto pool_sizes = MAP_VEC_ALLOCA(m_pool_sizes, [&](const std::pair<VkDescriptorType, float>& sizes) {
            return VkDescriptorPoolSize{
                .type            = sizes.first,
                .descriptorCount = static_cast<u32>(sizes.second * m_max_sets),
            };
        });

        VkDescriptorPoolCreateInfo info{
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = m_max_sets,
            .poolSizeCount = static_cast<u32>(pool_sizes.size()),
            .pPoolSizes    = pool_sizes.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device(), &info, nullptr, &m_current_pool));
    }
}

void DescriptorPool::reset() {
    VK_CHECK(vkResetDescriptorPool(device(), m_current_pool, 0));

    for (auto pool : m_used_pools) {
        VK_CHECK(vkResetDescriptorPool(device(), pool, 0));
        m_free_pools.push_back(pool);
    }

    m_used_pools.clear();
}

DescriptorPool::~DescriptorPool() {
    if (m_current_pool) vkDestroyDescriptorPool(device(), m_current_pool, nullptr);

    for (auto pool : m_free_pools)
        vkDestroyDescriptorPool(device(), pool, nullptr);

    for (auto pool : m_used_pools)
        vkDestroyDescriptorPool(device(), pool, nullptr);
}

} // namespace vke

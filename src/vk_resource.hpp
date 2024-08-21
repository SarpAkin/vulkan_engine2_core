#pragma once

#include "common.hpp" // IWYU pragma: export
#include "fwd.hpp"

#include <atomic>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace vke {

class DeviceGetter {
protected:
    VkDevice device() const;
    static VulkanContext* get_context();

    DeviceGetter()  = default;
    ~DeviceGetter() = default;
};

template <class T>
class RCResource;

class Resource : public DeviceGetter {
    template<typename T>
    friend class RCResource<T>;

public:
    Resource();
    virtual ~Resource();

    Resource(const Resource&)            = delete;
    Resource(Resource&&)                 = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&)      = delete;

    bool is_reference_counted() const { return m_is_reference_counted; }
    //Must be refence counted or else it will assert
    RCResource<Resource> get_reference();

protected:
private:
    void increment_reference_count() {
        m_ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    std::atomic<int> m_ref_count = 0;
    bool m_is_reference_counted  = false;
};

template <class T>
class RCResource {
    static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

    friend Resource;
public:
    T* get() { return m_ptr; }
    const T* get() const { return m_ptr; }

    T* operator->() { return m_ptr; }
    const T* operator->() const { return m_ptr; }

    RCResource(std::unique_ptr<T>&& _res) {
        reset(std::move(_res));
    }

    RCResource& operator=(std::unique_ptr<T>&& _res) {
        // Release the current resource
        release();

        // Take ownership of the new resource
        reset(std::move(_res));
        return *this;
    }

    RCResource(const RCResource& other) {
        m_ptr = other.m_ptr;
        if (m_ptr) {
            m_ptr->increment_ref_counter();
        }
    }

    // Copy assignment operator
    RCResource& operator=(const RCResource& other) {
        if (this != &other) {
            // Release the current resource
            release();

            // Copy the new resource
            m_ptr = other.m_ptr;
            if (m_ptr) {
                m_ptr->increment_ref_counter();
            }
        }
        return *this;
    }

    // Move constructor
    RCResource(RCResource&& other) noexcept
        : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    ~RCResource() {
        release();
    }

    template <class T1>
    RCResource<T1> convert_to() {
        if (m_ptr == nullptr) return RCResource(nullptr);

        m_ptr->increment_ref_counter();

        return RCResource(m_ptr);
    }

private:
    // Unsafe consturctor that doesn't increment reference count
    RCResource(T* ptr) : m_ptr(ptr) {
    }

private:
    void release() {
        if (m_ptr) {
            if (m_ptr->m_ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // Last reference, delete the resource
                delete m_ptr;
            }
            m_ptr = nullptr;
        }
    }

    // Helper function to reset the resource
    void reset(std::unique_ptr<T>&& _res) {
        T* res_typed  = _res.release();
        Resource* res = res_typed;

        assert(res->m_is_reference_counted == false);

        res->m_is_reference_counted = true;
        res->m_ref_count            = 1;

        m_ptr = res;
    }

private:
    T* m_ptr = nullptr;
};

} // namespace vke
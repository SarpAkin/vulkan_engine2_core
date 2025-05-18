#pragma once

#include "common.hpp" // IWYU pragma: export
#include "fwd.hpp"

#include <assert.h>
#include <atomic>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace vke {

class DeviceGetter {
protected:
    VkDevice device() const;
    const vk::detail::DispatchLoaderDynamic& get_dispatch_table() const;
    // shorthand for get_dispatch_table
    const vk::detail::DispatchLoaderDynamic& dt() const { return get_dispatch_table(); }

    static VulkanContext* get_context();

public:
    DeviceGetter()  = default;
    ~DeviceGetter() = default;
};

template <typename T>
class RCResource;

class Resource : public DeviceGetter {
    enum class OwnerShip {
        OWNED,
        RefCounted,
        // external behaves like owned but when get_reference is called it returns the external resource it points to
        EXTERNAL,
    };

    template <typename T>
    friend class RCResource;

public:
    Resource();
    virtual ~Resource();

    Resource(const Resource&)            = delete;
    Resource(Resource&&)                 = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&)      = delete;

    bool is_reference_counted() const { return m_ownership == OwnerShip::RefCounted; }
    // Must be reference counted or else it will assert
    RCResource<Resource> get_reference();

    void set_external(Resource* external) {
        m_external  = external;
        m_ownership = OwnerShip::EXTERNAL;
    }

    RCResource<Resource> try_get_reference();

protected:
private:
    void increment_reference_count() {
        m_ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    std::atomic<int> m_ref_count = 0;
    OwnerShip m_ownership        = OwnerShip::OWNED;

    Resource* m_external = nullptr;
};

template <class T>
class RCResource {

    friend Resource;

public:
    T* get() { return m_ptr; }
    const T* get() const { return m_ptr; }

    T* operator->() { return m_ptr; }
    const T* operator->() const { return m_ptr; }

    RCResource() {
        m_ptr = nullptr;
    }

    RCResource(T* ptr) {
        static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

        if (ptr == nullptr) {
            m_ptr = nullptr;
            return;
        }

        Resource* res = ptr;
        if (res->m_ownership != Resource::OwnerShip::RefCounted) {
            throw std::runtime_error("can't construct RCResource from Raw pointer as the pointed resource isn't ref counted");
        }

        res->increment_reference_count();
        m_ptr = ptr;
    }

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
            m_ptr->increment_reference_count();
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
                m_ptr->increment_reference_count();
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

        m_ptr->increment_reference_count();

        return RCResource<T1>(m_ptr);
    }

    bool is_null() const { return m_ptr == nullptr; }

    operator bool() const { return !is_null(); }

    bool operator==(const RCResource& other) const { return m_ptr == other.m_ptr; }
    bool operator==(const T* ptr) const { return m_ptr == ptr; }

private:
    static RCResource create_from_raw_pointer(T* ptr) {
        RCResource res;
        res.m_ptr = ptr;
        return res;
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

        assert(res->is_reference_counted() == false);

        if (res->m_ownership == Resource::OwnerShip::OWNED) res->m_ownership = Resource::OwnerShip::RefCounted;
        res->m_ref_count = 1;

        m_ptr = res_typed;
    }

private:
    T* m_ptr = nullptr;
};

} // namespace vke
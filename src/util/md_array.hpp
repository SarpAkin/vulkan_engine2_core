#pragma once

#include "../common.hpp"
#include <array>
#include <limits>
#include <memory>
#include <span>

namespace vke {

template <class TValue, usize Dim>
class MDArray {
private:
    constexpr static size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

public:
    using key_type       = std::array<u32, Dim>;
    using value_type     = TValue;
    using allocator_type = std::allocator<value_type>;

#pragma region ctor/dtor
public: // c'tors / d'tors
    MDArray(const key_type& size, const TValue& initial_value = TValue()) {
        m_sizes          = size;
        size_t flat_size = calculate_flat_size();

        m_data = _allocate<TValue>(flat_size);

        for (int i = 0; i < flat_size; i++) {
            std::construct_at(&m_data[i], initial_value);
        }
    }

    MDArray() {
        m_data  = nullptr;
        m_sizes = std::array<u32, Dim>(0);
    }

    MDArray(const MDArray& other) {
        m_sizes          = other.m_sizes;
        size_t flat_size = calculate_flat_size();

        m_data = _allocate<TValue>(flat_size);
        for (size_t i = 0; i < flat_size; i++) {
            std::construct_at(&m_data[i], other.m_data[i]);
        }
    }

    MDArray& operator=(const MDArray& other) {
        if (this == &other) return *this;

        _destroy();

        m_sizes          = other.m_sizes;
        size_t flat_size = calculate_flat_size();

        m_data = _allocate<TValue>(flat_size);
        for (size_t i = 0; i < flat_size; i++) {
            std::construct_at(&m_data[i], other.m_data[i]);
        }
        return *this;
    }

    MDArray(MDArray&& other) noexcept {
        m_sizes = other.m_sizes;
        m_data  = other.m_data;

        other.m_data = nullptr;
        other.m_sizes.fill(0);
    }

    MDArray& operator=(MDArray&& other) noexcept {
        if (this == &other) return *this;

        _destroy();

        m_sizes = other.m_sizes;
        m_data  = other.m_data;

        other.m_data = nullptr;
        other.m_sizes.fill(0);
        return *this;
    }

    ~MDArray() {
        _destroy();
    }
#pragma region lookup
public: // lookup
    value_type& operator[](const key_type& index) { return m_data[convert_to_flat_index(index)]; }
    const value_type& operator[](const key_type& index) const { return m_data[convert_to_flat_index(index)]; }

    value_type& at(const key_type& index) { return m_data[convert_to_flat_index_checked_throws(index)]; }
    const value_type& at(const key_type& index) const { return m_data[convert_to_flat_index_checked_throws(index)]; }

#pragma region util
public: // util
    std::span<value_type> as_flat_span() { return std::span<value_type>(m_data, calculate_flat_size()); }
    std::span<const value_type> as_flat_span() const { return std::span<value_type>(m_data, calculate_flat_size()); }

    void foreach (auto&& f) {
        if constexpr (requires(value_type v, key_type k) { f(v, k); }) {
            _indexed_foreach(f);
        } else {
            for (size_t i = 0, size = calculate_flat_size(); i < size; i++) {
                f(m_data[i]);
            }
        }
    }

    void foreach (auto&& f) const {
        if constexpr (requires(value_type v, key_type k) { f(v, k); }) {
            _indexed_foreach<true>(f);
        } else {
            for (size_t i = 0, size = calculate_flat_size(); i < size; i++) {
                f(m_data[i]);
            }
        }
    }

private:
    void _destroy() {
        if (m_data == nullptr) return;

        size_t flat_size = calculate_flat_size();
        for (size_t i = 0; i < flat_size; i++) {
            std::destroy_at(&m_data[i]);
        }
        _deallocate(m_data, flat_size);
        m_data = nullptr;
    }

private:
    size_t convert_to_flat_index(const key_type& md_index) const {
        size_t final_index = 0;
        for (int i = 0; i < Dim; i++) {
            final_index = final_index * m_sizes[i] + md_index[i];
        }
        return final_index;
    }

    size_t convert_to_flat_index_checked(const key_type& md_index) const {
        size_t final_index = 0;
        for (int i = 0; i < Dim; i++) {
            if (md_index >= m_sizes[i]) return INVALID_INDEX;

            final_index = final_index * m_sizes[i] + md_index[i];
        }
        return final_index;
    }

    size_t convert_to_flat_index_checked_throws(const key_type& md_index) const {
        size_t index = convert_to_flat_index_checked(md_index);
        if (index == INVALID_INDEX) throw std::out_of_range("MDArray out of range");
        return index;
    }

    size_t calculate_flat_size() const {
        size_t total_size = 1;
        for (int i = 0; i < Dim; i++) {
            total_size *= m_sizes[i];
        }
        return total_size;
    }

    template <bool is_const = false>
    void _indexed_foreach(auto&& f) {
        key_type md_indices;
        _indexed_foreach<0, is_const>(f, 0, md_indices);
    }

    template <size_t N, bool is_const = false>
    void _indexed_foreach(auto&& f, size_t accumulated_flat_index, key_type& md_indices) {
        u32& i = md_indices[N];
        for (i = 0; i < m_sizes[N]; i++) {
            size_t flat_index = accumulated_flat_index + i;
            if constexpr (N == Dim) {
                if constexpr (is_const) {
                    f(const_cast<const value_type&>(m_data[flat_index]), md_indices);
                } else {
                    f(m_data[flat_index], md_indices);
                }
            } else {
                indexed_foreach<N + 1>(f, flat_index, md_indices);
            }
        }
    }

#pragma region allocator
private: // allocator
    allocator_type _get_allocator() const { return allocator_type(); }

    template <class T>
    auto _get_allocator2() const {
        using alloc_T = std::allocator_traits<allocator_type>::template rebind_alloc<T>;

        return alloc_T(_get_allocator());
    }

    template <class T>
    T* _allocate(size_t n) { return _get_allocator2<T>().allocate(n); }
    template <class T>
    void _deallocate(T* ptr, size_t n) { _get_allocator2<T>().deallocate(ptr, n); }

private:
    TValue* m_data = nullptr;
    key_type m_sizes;
};

} // namespace vke

#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <optional>
#include <utility>

namespace vke {

template <class T, bool smallVec = false>
class SlimVec {
public:
    using iterator       = T*;
    using const_iterator = const T*;

public: // c'tors
    SlimVec() = default;
    SlimVec(const SlimVec& other) { _copy_from(other); }
    SlimVec(SlimVec&& other) { _move_from(std::move(other)); }
    ~SlimVec() { _cleanup(); }

    SlimVec& operator=(const SlimVec& other) {
        _copy_from(other);
        return other;
    }
    SlimVec& operator=(SlimVec&& other) {
        _move_from(std::move(other));
        return *this;
    }

public:
    T* data(T* data) { return _data(); }
    const T* data(T* data) const { return _data(); }

    T& operator[](size_t index) { return _data()[index]; }
    const T& operator[](size_t index) const { return _data()[index]; }

    iterator begin() { return _data(); }
    iterator end() { return _data() + _size(); }
    const_iterator begin() const { return _data(); }
    const_iterator end() const { return _data() + _size(); }

    void push_back(T&& item) {
        auto cur_size = _size();

        _ensure_capacity_for_push_back();

        new (_data() + cur_size) T(std::move(item));
        _set_size(cur_size + 1);
    }

    void push_back(const T& item) {
        auto cur_size = _size();

        _ensure_capacity_for_push_back();

        new (_data() + cur_size) T(std::move(item));
        _set_size(cur_size + 1);
    }

    void reserve(size_t new_capacity) { _set_capacity(std::max(static_cast<uint32_t>(new_capacity), _size())); }

    void resize(size_t __new_size, const T& value = T()) {
        uint32_t new_size = __new_size;
        uint32_t old_size = _size();
        if (old_size == new_size) return;

        if (_capacity() < new_size) {
            _set_capacity(new_size);
        }

        _set_size(new_size);

        T* data = _data();

        if (new_size > old_size) {
            for (int i = old_size; i < new_size; i++) {
                new (data + i) T(value);
            }
        } else {
            for (int i = new_size; i < old_size; i++) {
                data[i].~T();
            }
        }
    }

    T pop_back() {
        uint32_t cur_size = _size();
        assert(cur_size > 0);

        auto cur_size_m1 = cur_size - 1;

        T item = std::move(_data()[cur_size_m1]);
        _data()[cur_size_m1].~T();
        _set_size(cur_size_m1);

        return item;
    }

    void insert(const_iterator pos, const T& value) {
        insert(std::distance(begin(), pos), value);
    }

    void insert(const_iterator pos, T&& value) {
        insert(std::distance(begin(), pos), value);
    }

    void insert_at(size_t index, T&& value) {
        _ensure_capacity_for_push_back();

        auto* data = _data();
        for (size_t i = _size(); i > index; --i) {
            data[i] = std::move(data[i - 1]);
        }

        new(data + index) T(std::move(value));
        _set_size(_size() + 1);
    }

    void insert_at(size_t index, const T& value) {
        _ensure_capacity_for_push_back();

        auto* data = _data();
        for (size_t i = _size(); i > index; --i) {
            data[i] = std::move(data[i - 1]);
        }

        new(data + index) T(value);
        _set_size(_size() + 1);
    }

    void erase(const_iterator first) {
        erase(first, first + 1);
    }

    void erase(const_iterator first, const_iterator last) {
        erase_at(std::distance(begin(), first), std::distance(first, last));
    }

    void erase_at(size_t index, size_t count = 1) {
        assert(index + count <= _size());

        size_t size = _size();
        auto* data = _data();
        
        // shift elements
        for(size_t i = index + count; i < size; ++i) {
            data[i - count] = std::move(data[i]);
        }

        // destruct last elements
        for(size_t i = size - count; i < size; ++i) {
            data[i].~T();
        }

        _set_size(size - count);
    }

    std::optional<T> try_pop_back() {
        if (_size() == 0) return std::nullopt;

        return pop_back();
    }

    size_t capacity() const { return _capacity(); }
    size_t size() const { return _size(); }

    void clear() {
        for (int i = 0; i < m_size; ++i) {
            _data()[i].~T();
        }

        m_size = 0;
    }

    bool empty() const { return m_size == 0; }
    void shrink_to_fit() { /*TODO*/ }

private:
    void _ensure_capacity_for_push_back() {
        if (_size() == _capacity()) {
            _set_capacity(_calculate_new_capacity());
        }
    }

    size_t _calculate_new_capacity() const {
        size_t current_cap = _capacity();

        if (current_cap == 0) {
            return 4;
        }

        return (current_cap * 3 + 1) / 2;
    }

private:
    constexpr static size_t _small_vec_byte_space    = offsetof(SlimVec, m_size);
    constexpr static size_t _small_vec_item_capacity = _small_vec_byte_space / sizeof(T);
    constexpr static bool _small_vec                 = smallVec && _small_vec_item_capacity >= 1;
    constexpr static uint32_t _small_vec_base_size   = UINT32_MAX - (_small_vec_item_capacity + 1);

private:
    void _cleanup() {
        T* data = _data();

        uint32_t cur_size = _size();

        for (int i = 0; i < cur_size; ++i) {
            data[i].~T();
        }

        if (!is_small_vec()) {
            free(data);
        }

        m_data_ptr = nullptr;
        m_capacity = 0;
        m_size     = 0;
    }

    void _move_from(SlimVec&& other) {
        _cleanup();

        if (other.is_small_vec()) {
            m_size = other.m_size;

            T* data = _data();

            for (int i = 0; i < other._size(); ++i) {
                new (data + i) T(std::move(other[i]));
                other[i].~T();
            }

            other.m_size = 0;
        } else {
            m_data_ptr = other.m_data_ptr;
            m_capacity = other.m_capacity;
            m_size     = other.m_size;

            other.m_data_ptr = nullptr;
            other.m_capacity = 0;
            other.m_size     = 0;
        }
    }

    void _copy_from(const SlimVec& other) {
        _cleanup();

        uint32_t o_size = other._size();

        _set_capacity(o_size);

        _set_size(o_size);

        T* data = _data();

        for (int i = 0; i < o_size; ++i) {
            new (data + i) T(other[i]);
        }
    }

private:
    T* _data() const {
        if (is_small_vec()) {
            return const_cast<T*>(reinterpret_cast<const T*>(&m_data_ptr));
        }

        return m_data_ptr;
    }

    uint32_t _size() const {
        if constexpr (_small_vec) {
            return m_size >= _small_vec_base_size ? m_size - _small_vec_base_size : m_size;
        }
        return m_size;
    }

    void _set_capacity(size_t min_capacity) {
        bool is_small_vec_at_start = is_small_vec();

        if (is_small_vec_at_start && min_capacity <= _small_vec_item_capacity) {
            return;
        }

        uint32_t new_capacity = min_capacity;

        T* new_data = reinterpret_cast<T*>(malloc(new_capacity * sizeof(T)));

        T* old_data       = _data();
        uint32_t cur_size = _size();

        if (is_small_vec_at_start) {
            m_size = cur_size; // we set it normally to make it non small vec
        }

        for (int i = 0; i < cur_size; ++i) {
            new (new_data + i) T(std::move(old_data[i]));
            old_data[i].~T();
        }

        if (old_data != nullptr && !is_small_vec_at_start) {
            free(old_data);
        }

        m_data_ptr = new_data;
        m_capacity = new_capacity;
    }

    size_t _capacity() const {
        return is_small_vec() ? _small_vec_item_capacity : m_capacity;
    }

    bool is_small_vec() const {
        if constexpr (_small_vec) {
            return m_size >= _small_vec_base_size;
        } else {
            return false;
        }
    }

    // Does not resize the vector
    void _set_size(uint32_t size) {
        if (is_small_vec()) {
            assert(size <= _small_vec_item_capacity);

            m_size = size + _small_vec_base_size;
        } else {
            m_size = size;
        }
    }

private: // fields
    T* m_data_ptr       = nullptr;
    uint32_t m_capacity = 0, m_size = _small_vec ? _small_vec_base_size : 0;
};

template <class T, size_t MinStackSlots = 0>
using SmallVec = SlimVec<T, true>;

} // namespace vke

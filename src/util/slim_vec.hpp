#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vke {

template <class T, bool smallVec = false, size_t desired_stack_slots = 0>
class SlimVec {
public:
    using iterator       = T*;
    using const_iterator = const T*;

public: // c'tors
    SlimVec(size_t size, const T& val) { resize(size, val); }
    SlimVec(size_t size) { resize(size, T()); }

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

    template <class Container>
    SlimVec(Container&& val) { assign(std::forward<Container>(val)); }

    template <class Container>
    SlimVec& operator=(Container&& val) {
        assign(std::forward<Container>(val));
        return *this;
    }

    SlimVec& operator=(const std::initializer_list<T>& iterable) {
        assign(iterable);
        return *this;
    }

    SlimVec(const std::initializer_list<T>& iterable) { assign(iterable); }

public:
    void assign(const SlimVec& other) { _copy_from(other); }
    void assign(SlimVec&& other) { _move_from(std::move(other)); }

    void assign(const std::vector<T>& std_vec) {
        clear();
        reserve(std_vec.size());
        for (const auto& e : std_vec) {
            push_back(e);
        }
    }

    void assign(const std::initializer_list<T>& iterable) {
        clear();
        reserve(std::distance(iterable.begin(), iterable.end()));
        for (const auto& e : iterable) {
            push_back(e);
        }
    }

    template <class Iterable>
    void assign(Iterable&& iterable) {
        clear();
        reserve(std::distance(iterable.begin(), iterable.end()));
        for (const auto& e : iterable) {
            push_back(e);
        }
    }

public:
    T* data() { return _data(); }
    const T* data() const { return _data(); }

    T& operator[](size_t index) { return _data()[index]; }
    const T& operator[](size_t index) const { return _data()[index]; }

    T& at(size_t index) {
        if (index >= size()) throw std::out_of_range("out of range at method vke::SlimVec::at(size_t)");
        return _data()[index];
    }

    const T& at(size_t index) const {
        if (index >= size()) throw std::out_of_range("out of range at method vke::SlimVec::at(size_t)");
        return _data()[index];
    }

    iterator begin() { return _data(); }
    iterator end() { return _data() + _size(); }
    const_iterator begin() const { return cbegin(); }
    const_iterator end() const { return cend(); }
    const_iterator cbegin() const { return _data(); }
    const_iterator cend() const { return _data() + _size(); }

    operator std::span<T>() { return {_data(), _size()}; }
    operator std::span<const T>() const { return {_data(), _size()}; }

    std::span<T> as_span() { return {data(), size()}; }
    std::span<const T> as_const_span() { return {data(), size()}; }

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

    void push_back_unchecked(T&& item) {
        auto cur_size = _size();

        assert(cur_size < _capacity());

        new (_data() + cur_size) T(std::move(item));
        _set_size(cur_size + 1);
    }

    void push_back_unchecked(const T& item) {
        auto cur_size = _size();

        assert(cur_size < _capacity());

        new (_data() + cur_size) T(std::move(item));
        _set_size(cur_size + 1);
    }

    template <class... Args>
    void emplace_back(Args&&... args) {
        auto cur_size = _size();

        _ensure_capacity_for_push_back();

        new (_data() + cur_size) T(std::forward<Args>(args)...);
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
            for (uint32_t i = old_size; i < new_size; i++) {
                new (data + i) T(value);
            }
        } else {
            for (uint32_t i = new_size; i < old_size; i++) {
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

    template <class... Args>
    void emplace(const_iterator pos, Args&&... args) {
        insert(pos, T(std::forward<Args>(args)...));
    }

    void insert(const_iterator pos, const T& value) {
        insert_at(std::distance(begin(), pos), value);
    }

    void insert(const_iterator pos, T&& value) {
        insert_at(std::distance(begin(), pos), value);
    }

    void insert_at(size_t index, T&& value) {
        _ensure_capacity_for_push_back();

        auto* data = _data();
        new (data + _size()) T();

        for (size_t i = _size(); i > index; --i) {
            data[i] = std::move(data[i - 1]);
        }

        data[index] = std::move(value);

        _set_size(_size() + 1);
    }

    void insert_at(size_t index, const T& value) {
        _ensure_capacity_for_push_back();

        auto* data = _data();
        new (data + _size()) T();

        for (size_t i = _size(); i > index; --i) {
            data[i] = std::move(data[i - 1]);
        }

        data[index] = value;

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
        auto* data  = _data();

        // shift elements
        for (size_t i = index + count; i < size; ++i) {
            data[i - count] = std::move(data[i]);
        }

        // destruct last elements
        for (size_t i = size - count; i < size; ++i) {
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
        auto* data = _data();
        for (int i = 0; i < _size(); ++i) {
            data[i].~T();
        }

        _set_size(0);
    }

    void swap(SlimVec& other) { std::swap(*this, other); }

    T& back() { return *(end() - 1); }
    const T& back() const { return *(end() - 1); }
    T& front() { return *begin(); }
    const T& front() const { return *begin(); }

    bool empty() const { return _size() == 0; }
    void shrink_to_fit() { /*TODO*/ }
    constexpr size_t max_size() const { return _small_vec_base_size - 1; }

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
    void _cleanup() {
        T* data = _data();

        uint32_t cur_size = _size();

        for (uint32_t i = 0; i < cur_size; ++i) {
            data[i].~T();
        }

        if (!is_small_vec() && _capacity() > 0) {
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

        for (uint32_t i = 0; i < cur_size; ++i) {
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

    // Do not set m_size manually
    void _set_size(uint32_t size) {
        if (is_small_vec()) {
            assert(size <= _small_vec_item_capacity);

            m_size = size + _small_vec_base_size;
        } else {
            m_size = size;
        }
    }

private:
    static constexpr size_t calculate_data_array_size() {
        if (smallVec == false) return 0;

        size_t total_needed_space = (std::max(desired_stack_slots * sizeof(T), 12ul) - 12);

        return (total_needed_space + 7) / 8;
    }

private:
    constexpr static size_t _small_vec_byte_space    = offsetof(SlimVec, m_size);
    constexpr static size_t _small_vec_item_capacity = _small_vec_byte_space / sizeof(T);
    constexpr static bool _small_vec                 = smallVec && _small_vec_item_capacity >= 1;
    constexpr static uint32_t _small_vec_base_size   = UINT32_MAX - (_small_vec_item_capacity + 1);

private: // fields
    T* m_data_ptr = nullptr;
    // on the small vec mode, from the start of m_data_ptr to begining of m_size is used for storage
    size_t m_extra_dummy_storage[calculate_data_array_size()];
    uint32_t m_capacity = 0, m_size = _small_vec ? _small_vec_base_size : 0;
};

template <class T, size_t MinStackSlots = 0>
using SmallVec = SlimVec<T, true, MinStackSlots>;

} // namespace vke

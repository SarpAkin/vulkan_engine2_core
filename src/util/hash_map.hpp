#pragma once

#include <cassert>
#include <cstdint>
#include <numeric>
#include <span>
#include <stdexcept>

namespace vke {

namespace impl {
template <class T>
void __custom_qsort(std::span<T> range, size_t offset, auto&& less, auto&& on_swap, int depth = 0) {
    auto swap_in_span = [&](size_t a, size_t b) {
        std::swap(range[a], range[b]);
        on_swap(offset + a, offset + b);
    };

    if (range.size() == 2) {
        if (!less(range[0], range[1])) {
            swap_in_span(0, 1);
        }
        return;
    } else if (range.size() <= 1) {
        return;
    }

    // randomize the pivot occasionally to prevent worse case O(n^2) from happening
    if ((range.size() > 20) || (depth == 0)) swap_in_span(0, rand() % range.size());

    size_t begin = 1, end = range.size() - 1;
    T& pivot = range[0];

    while (begin < end) {
        while (less(range[begin], pivot) && begin < range.size()) {
            begin++;
        }

        while (!less(range[end], pivot) && begin < end) {
            end--;
        }

        if (begin < end) {
            swap_in_span(begin, end);
        }
    }

    // elements bigger than pivot should be after end
    // we move the pivot to middle
    if (begin >= 2) {
        size_t pivot_index = begin - 1;
        swap_in_span(0, pivot_index);

        // don't include the pivot
        __custom_qsort(range.subspan(0, pivot_index), offset, less, on_swap, depth + 1);
    }

    __custom_qsort(range.subspan(begin), begin + offset, less, on_swap, depth + 1);
}

} // namespace impl

template <class T>
void custom_qsort(std::span<T> range, auto&& less = std::less<T>(), auto&& on_swap = [](size_t, size_t) {}) {
    impl::__custom_qsort(range, 0, less, on_swap);
}

template <class K, class V, class Hasher = std::hash<K>, class KeyEqual = std::equal_to<K>>
class DenseHashMap {
public: // fwds
    template <class ValueType>
    class Iterator;

#pragma mark #member_types
public: // member types
    using size_type        = uint32_t;
    using key_type         = K;
    using mapped_type      = V;
    using value_type       = std::pair<K, V>;
    using const_value_type = std::pair<const K, const V>;
    using hasher           = Hasher;
    using key_equal        = KeyEqual;
    using allocator_type   = std::allocator<value_type>;
    using reference        = std::pair<const K&, V&>;
    using const_reference  = std::pair<const K&, const V&>;
    using iterator         = Iterator<reference>;
    using const_iterator   = Iterator<const_reference>;

#pragma mark #ctors
public: // c'tors
    DenseHashMap() {}
    DenseHashMap(std::initializer_list<value_type> ilist) { insert(std::move(ilist)); }
    DenseHashMap(DenseHashMap&& other) { _assign(std::move(other)); }
    DenseHashMap& operator=(DenseHashMap&& other) {
        _assign(std::move(other));
        return *this;
    }

    ~DenseHashMap() { cleanup(); }

#pragma mark #iterators
public: // iterators
    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, m_kv_pair_count); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, m_kv_pair_count); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, m_kv_pair_count); }

#pragma mark #capacity
public: // capacity
    bool empty() const { return m_kv_pair_count == 0; }
    size_type size() const { return m_kv_pair_count; }
    size_type max_size() const { std::numeric_limits<size_type>::max(); }

#pragma mark #modifier
public: // modifiers
    void clear() { _clear(); }

    std::pair<iterator, bool> insert(value_type&& x) { return _insert_value_type(std::move(x), false); }
    std::pair<iterator, bool> insert(const value_type& x) { return _insert_value_type(x, false); }
    std::pair<iterator, bool> insert(iterator, value_type&& x) { return _insert_value_type(std::move(x), false); }
    std::pair<iterator, bool> insert(iterator, const value_type& x) { return _insert_value_type(x, false); }
    std::pair<iterator, bool> insert(const_iterator, value_type&& x) { return _insert_value_type(std::move(x), false); }
    std::pair<iterator, bool> insert(const_iterator, const value_type& x) { return _insert_value_type(x, false); }
    void insert(std::initializer_list<value_type> ilist) { return _insert_it_range(ilist.begin(), ilist.end(), false); }
    template <class InputIt>
    void insert(InputIt b, InputIt e) { _insert_it_range(b, e, false); }

    void insert_or_assign(const K& k, V&& v) { _insert(k, v, true); }

    size_type erase(const K& key) { return _erase(key); }

#pragma mark #lookup
public: // lookup
    V& at(const K& key) { return _at<false>(key); }
    const V& at(const K& key) const { return _const_at(key); }

    V& operator[](const K& key) { return _at<true>(key); }
    size_type count(const K& key) { return _contains(key) ? 1 : 0; }

    iterator find(const K& key) { return _find<iterator>(key); }
    const_iterator find(const K& key) const { return _find<const_iterator>(key); }

    bool contains(const K& key) { return _contains(key); }

#pragma mark #hash_policy
public: // hash policy
    void rehash() { _rehash_all(); }
    void reserve(size_type n) { _reserve(n); }

#pragma mark #observers
public: // observers
    hasher hash_function() const { return hasher(); }
    key_equal key_eq() const { return key_equal(); }

#pragma mark #extras
public:
    // extra functions that are not defined in std::unordered_map

    // sorts by ascending order when comparison is defined for less
    void sort_by_key(auto&& less) {
        size_t swap_count   = 0;
        size_type cmp_count = 0;

        auto less2 = [&](const auto& a, const auto& b) {
            cmp_count++;
            return less(a, b);
        };

        custom_qsort(std::span(m_keys, m_kv_pair_count), less2, [&](size_t a, size_t b) {
            std::swap(m_values[a], m_values[b]);
            swap_count++;
        });

        printf("swapped %d times\n", static_cast<int>(swap_count));
        printf("compared %d times\n", static_cast<int>(cmp_count));
        _rehash_all();
    }

#pragma mark #iterator_implement
public: // Iterator implement
    template <class ValueType>
    class Iterator {
        friend DenseHashMap;

        // An helper class to make the -> operator possible
        class Proxy {
        public:
            explicit Proxy(const ValueType& v) : m_value(v) {}
            const ValueType* operator->() const { return &m_value; }

        private:
            ValueType m_value;
        };

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = ValueType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = ValueType*;
        using reference         = ValueType&;

    private:
        Iterator(const DenseHashMap* map, difference_type kv_index) : m_map(map), m_kv_index(kv_index) {}

    public:
        Iterator(const Iterator&)            = default;
        Iterator& operator=(const Iterator&) = default;

        ValueType operator*() const { return _get(); }
        Proxy operator->() const { return Proxy(_get()); }

        Iterator operator++(int) {
            auto old = *this;
            m_kv_index++;
            return old;
        }

        Iterator& operator++() {
            m_kv_index++;
            return *this;
        }

        Iterator operator--(int) {
            auto old = *this;
            m_kv_index--;
            return old;
        }

        Iterator& operator--() {
            m_kv_index--;
            return *this;
        }

        bool operator==(const Iterator& other) const { return m_kv_index == other.m_kv_index; }
        bool operator!=(const Iterator& other) const { return !(*this == other); }
        difference_type operator-(const Iterator& other) const { return m_kv_index - other.m_kv_index; }

        Iterator operator+(difference_type offset) const { return Iterator(m_map, m_kv_index + offset); }
        Iterator operator-(difference_type offset) const { return Iterator(m_map, m_kv_index - offset); }

        Iterator& operator+=(difference_type offset) {
            m_kv_index += offset;
            return *this;
        }

        Iterator& operator-=(difference_type offset) {
            m_kv_index -= offset;
            return *this;
        }

    private:
        ValueType _get() const { return ValueType(m_map->_keys_ptr()[m_kv_index], m_map->_values_ptr()[m_kv_index]); }

    public:
    protected:
        const DenseHashMap* m_map  = nullptr;
        difference_type m_kv_index = 0;
    };

private:
    static constexpr size_type empty_kv_index         = std::numeric_limits<size_type>::max();
    static constexpr size_type erased_kv_index        = std::numeric_limits<size_type>::max() - 1;
    static constexpr size_type initial_capacity       = 7;
    static constexpr size_type null_index_table_index = std::numeric_limits<size_type>::max();

    static constexpr inline size_t hash_key(const K& k) { return Hasher()(k); }
    static constexpr inline bool keys_equal(const K& a, const K& b) { return KeyEqual()(a, b); }

#pragma mark #modifier_implement
private: // modifier implement
    void _clear(bool clear_index_table = true) {

        K* keys   = _keys_ptr();
        V* values = _values_ptr();

        for (int i = 0; i < m_kv_pair_count; i++) {
            keys[i].~K();
            values[i].~V();
        }

        m_kv_pair_count = 0;

        if (!clear_index_table) return;

        for (int i = 0; i < m_index_table_size; i++) {
            m_index_table[i] = empty_kv_index;
        }
    }

    std::pair<iterator, bool> _insert_value_type(value_type&& x, bool allow_override) {
        return _insert(std::move(x.first), std::move(x.second), allow_override);
    }

    std::pair<iterator, bool> _insert_value_type(const value_type& x, bool allow_override) {
        return _insert(x.first, V(x.second), allow_override);
    }

    std::pair<iterator, bool> _insert(K&& key, V&& value, bool allow_override) {
        if (m_index_table_size == 0) _reserve(initial_capacity);

        auto [index, key_exists] = find_table_index(key);

        if (index == null_index_table_index) {
            expand();
            return _insert(std::forward<K>(key), std::forward<V>(value), allow_override);
        }

        if (key_exists) {
            size_type kv_index = m_index_table[index];
            if (!allow_override) return std::pair(iterator(this, kv_index), false);

            _values_ptr()[kv_index] = std::forward<V>(value);
            return std::pair(iterator(this, kv_index), true);
        } else {
            if (!can_push_to_kv_storage()) {
                expand();
                return _insert(std::forward<K>(key), std::forward<V>(value), allow_override);
            }

            size_type kv_index   = push_back_to_kv_storage(value_type(key, std::forward<V>(value)));
            m_index_table[index] = kv_index;
            return std::pair(iterator(this, kv_index), true);
        }
    }

    std::pair<iterator, bool> _insert(const K& key, V&& value, bool allow_override) {
        if (m_index_table_size == 0) _reserve(initial_capacity);

        auto [index, key_exists] = find_table_index(key);

        if (index == null_index_table_index) {
            expand();
            return _insert(key, std::forward<V>(value), allow_override);
        }

        if (key_exists) {
            size_type kv_index = m_index_table[index];
            if (!allow_override) return std::pair(iterator(this, kv_index), false);

            _values_ptr()[kv_index] = std::forward<V>(value);
            return std::pair(iterator(this, kv_index), true);
        } else {
            if (!can_push_to_kv_storage()) {
                expand();
                return _insert(key, std::forward<V>(value), allow_override);
            }

            size_type kv_index   = push_back_to_kv_storage(value_type(key, std::forward<V>(value)));
            m_index_table[index] = kv_index;
            return std::pair(iterator(this, kv_index), true);
        }
    }

    template <class InputIt>
    void _insert_it_range(InputIt b, InputIt e, bool allow_override = false) {
        if constexpr (requires { { e - b } -> std::convertible_to<std::ptrdiff_t>; }) {
            ptrdiff_t range_size = e - b;
            hint_capacity(range_size + size());
        }

        using Ref = decltype(*b);

        for (auto it = b; it != e; it++) {
            if constexpr (std::is_rvalue_reference_v<Ref>) {
                _insert_value_type(std::move(*it), allow_override);
            } else {
                _insert_value_type(*it, allow_override);
            }
        }
    }

    size_type _erase(const K& key) {
        auto [index, key_exists] = find_table_index(key);

        if (index == std::numeric_limits<size_type>::max() || !key_exists) {
            return 0;
        }

        size_type kv_index = m_index_table[index];
        erase_kv_pair(kv_index);
        m_index_table[index] = erased_kv_index;

        return 1;
    }

#pragma mark #lookup_implement
private: // lookup implement
    template <bool create_empty>
    V& _at(const K& key) {
        size_type kv_index = find_kv_index(key);

        if (kv_index == empty_kv_index) {
            if constexpr (create_empty) {
                auto [it, b] = _insert(key, V(), false);
                return (*it).second;
            } else {
                throw std::out_of_range("DenseHashMap: key doesn't exist");
            }
        }

        return _values_ptr()[kv_index];
    }

    const V& _const_at(const K& key) const {
        size_type kv_index = find_kv_index(key);

        if (kv_index == empty_kv_index) {
            throw std::out_of_range("DenseHashMap: key doesn't exist");
        }

        return _values_ptr()[kv_index];
    }

    // if It != const_iterator this function shouldn't be called as cons't
    template <class It>
    It _find(const K& key) const {
        size_type kv_index = find_kv_index(key);
        if (kv_index == empty_kv_index) {
            return It(const_cast<DenseHashMap*>(this), m_kv_pair_count);
        }
        return It(const_cast<DenseHashMap*>(this), kv_index);
    }

    bool _contains(const K& key) const {
        return find_kv_index(key) != empty_kv_index;
    }

#pragma mark #extras_impl
private: // extras
#pragma mark #util
private: // util
    void erase_kv_pair(size_type kv_index) {
        K* keys   = _keys_ptr();
        V* values = _values_ptr();

        size_type last_index = m_kv_pair_count - 1;
        if (kv_index != last_index) {
            auto [t_index, occupied] = find_table_index(keys[last_index]);
            assert(t_index < m_index_table_size && occupied);

            keys[kv_index]   = std::move(keys[last_index]);
            values[kv_index] = std::move(values[last_index]);

            // after shifting the last element to kv_indexes value update the last elements index table index
            m_index_table[t_index] = kv_index;
        }

        keys[last_index].~K();
        values[last_index].~V();
        m_kv_pair_count--;
    }

    size_type get_kv_storage_size() const { return m_index_table_size / 2; }
    // if it fails returns size_type::max
    size_type push_back_to_kv_storage(value_type&& x) {
        assert(can_push_to_kv_storage());

        new (&_keys_ptr()[m_kv_pair_count]) K(std::move(x.first));
        new (&_values_ptr()[m_kv_pair_count]) V(std::move(x.second));

        return m_kv_pair_count++;
    }

    bool can_push_to_kv_storage() const { return m_kv_pair_count < get_kv_storage_size(); }

    void cleanup() {
        _clear(false);

        _deallocate(m_index_table, m_index_table_size);
        m_index_table = nullptr;
        _deallocate(m_keys, get_kv_storage_size());
        m_keys = nullptr;
        _deallocate(m_values, get_kv_storage_size());
        m_values = nullptr;

        m_kv_pair_count    = 0;
        m_index_table_size = 0;
    }

    void hint_capacity(size_type s) {
        size_type initial_size = get_kv_storage_size();
        size_type desired_size = initial_size;

        while (s > desired_size) {
            desired_size = desired_size * 2 + 1;
        }

        if (initial_size != desired_size) {
            _reserve(desired_size);
        }
    }
    void expand() { _reserve(get_kv_storage_size() * 2 + 1); }

    // returns the index and whether it is occupied or not
    std::pair<size_type, bool> find_table_index(const K& key, bool find_insertion_point = false) const {
        if (m_index_table_size == 0) return std::pair(null_index_table_index, false);

        size_t hash = hash_key(key);

        // max index means that it doesn't have a value
        size_type insertable_index = null_index_table_index;

        size_type start = hash % m_index_table_size;
        size_type index = start;

        // probe as much as the index table
        for (size_type i = 0; i < m_index_table_size; i++, index = (start + i) % m_index_table_size) {

            // index of its key & value
            size_type kv_index = m_index_table[index];

            if (kv_index == empty_kv_index) {

                if (find_insertion_point && insertable_index != null_index_table_index) {
                    return std::pair(insertable_index, false);
                } else {
                    return std::pair(index, false);
                }
            } else if (kv_index == erased_kv_index) {
                if (find_insertion_point) insertable_index = index;
            } else if (keys_equal(key, _keys_ptr()[kv_index])) {
                return std::pair(index, true);
            }
        }

        return std::pair(null_index_table_index, false);
    }

    size_type find_kv_index(const K& key) const {
        auto [table_index, occupied] = find_table_index(key, false);
        if (!occupied) return empty_kv_index;

        if (table_index == null_index_table_index) return empty_kv_index;

        size_type kv_index = m_index_table[table_index];
        if (kv_index >= erased_kv_index) kv_index = empty_kv_index;

        return kv_index;
    }

    void _reserve(size_type new_kv_capacity) {
        assert(new_kv_capacity >= m_kv_pair_count);

        _deallocate(m_index_table, m_index_table_size);
        m_index_table = nullptr;

        K* old_key_array   = _keys_ptr();
        V* old_value_array = _values_ptr();

        K* new_key_array   = _allocate<K>(new_kv_capacity);
        V* new_value_array = _allocate<V>(new_kv_capacity);

        for (int i = 0; i < m_kv_pair_count; i++) {
            new (&new_key_array[i]) K(std::move(old_key_array[i]));
            new (&new_value_array[i]) V(std::move(old_value_array[i]));

            old_key_array[i].~K();
            old_value_array[i].~V();
        }

        _deallocate(old_key_array, get_kv_storage_size());
        _deallocate(old_value_array, get_kv_storage_size());

        m_keys   = new_key_array;
        m_values = new_value_array;

        m_index_table_size = new_kv_capacity * 2;
        m_index_table      = _allocate<size_type>(m_index_table_size);

        _rehash_all();
    }

    void _rehash_all() {
        for (int i = 0; i < m_index_table_size; i++) {
            m_index_table[i] = empty_kv_index;
        }

        K* keys = _keys_ptr();

        for (int i = 0; i < m_kv_pair_count; i++) {
            auto [index, key_exists] = find_table_index(keys[i], true);

            if (index == empty_kv_index) throw std::runtime_error("failed to rehash table!");

            m_index_table[index] = i;
        }
    }

    template <bool is_empty = false>
    void _assign(DenseHashMap&& other) {
        if (&other == this) return;

        if constexpr (!is_empty) {
            cleanup();
        }

        this->m_index_table_size = other.m_index_table_size;
        this->m_kv_pair_count    = other.m_kv_pair_count;
        this->m_index_table      = other.m_index_table;
        this->m_values           = other.m_values;
        this->m_keys             = other.m_keys;

        other.m_index_table_size = 0;
        other.m_kv_pair_count    = 0;
        other.m_index_table      = nullptr;
        other.m_values           = nullptr;
        other.m_keys             = nullptr;
    }

#pragma mark #allocator
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
    V* _values_ptr() const { return m_values; }
    K* _keys_ptr() const { return m_keys; }

private:
    size_type* m_index_table     = nullptr;
    K* m_keys                    = nullptr;
    V* m_values                  = nullptr;
    size_type m_index_table_size = 0, m_kv_pair_count = 0;
};

} // namespace vke
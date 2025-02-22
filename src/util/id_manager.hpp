#pragma once

#include <cstdint>
#include <stdexcept>

#include "slim_vec.hpp"

namespace vke {

template <typename T>
class IDManager {
public:
    uint64_t free_id_count() const { return m_freed_ids.size() + (std::numeric_limits<T>::max() - m_id_counter); }

    uint64_t id_count() const { return m_id_counter; }

    T new_id() {
        if (m_freed_ids.size() > 0) {
            T id = m_freed_ids.back();
            m_freed_ids.pop_back();
            return id;
        }

        uint64_t id = m_id_counter++;
        if (id > std::numeric_limits<T>::max()) throw std::runtime_error("id overflow");

        return static_cast<T>(id);
    }

    void fill_ids(auto& vec, int new_size) {
        if (vec.size() == new_size) return;

        if (vec.size() < new_size) {
            int old_size = vec.size();
            vec.resize(new_size);
            for (int i = old_size; i < new_size; i++) {
                vec[i] = new_id();
            }
        } else {
            for (int i = new_size; i < vec.size(); i++) {
                free_id(vec[i]);
            }
            vec.resize(new_size);
        }
    }

    void free_id(T id) {
        m_freed_ids.push_back(id);
    }

private:
    vke::SlimVec<T> m_freed_ids;
    uint64_t m_id_counter = 0;
};

} // namespace vke

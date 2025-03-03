#pragma once

#include <functional>

namespace std {
    template <typename T1, typename T2>
    struct hash<std::pair<T1, T2>> {
        size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            // Combine the two hash values
            return h1 ^ (h2 << 1); // Shift the second hash and XOR it with the first
        }
    };
}

#pragma once

#include <alloca.h>
#include <cassert>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "../common.hpp"
#include "../fwd.hpp"

namespace vke{
class ArenaAllocator;

auto map_vec(auto&& vector, auto&& f) -> std::vector<decltype(f(*vector.begin()))> {
    std::vector<decltype(f(*vector.begin()))> results;
    results.reserve(vector.size());
    for (const auto& element : vector) {
        results.push_back(f(element));
    }

    return results;
}

auto map_vec_indicies(auto&& vector, auto&& f) -> std::vector<decltype(f(*vector.begin(), 0))> {
    std::vector<decltype(f(*vector.begin(), 0))> results;
    results.reserve(vector.size());
    int i = 0;
    for (const auto& element : vector) {
        results.push_back(f(element, i));
        i++;
    }

    return results;
}

template <typename T>
auto map_optional(const std::optional<T>& opt, auto&& func) -> std::optional<decltype(func(*opt))> {
    if (opt) {
        return std::make_optional(func(*opt));
    }
    return std::nullopt;
}

std::vector<u8> read_file_binary(const char* name);
std::span<u8> read_file_binary(vke::ArenaAllocator* arena, const char* name);

std::string read_file(const char* name);
std::string_view read_file(vke::ArenaAllocator* arena, const char* name);

std::span<u32> cast_u8_to_span_u32(std::span<u8> span);


std::string relative_path_impl(const char* source_path, const char* path);

}



#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define TODO() assert(!"TODO!")

// auto map_vec_to_ptr_buffer(void* ptr,auto&& vector, auto&& f) -> std::vector<decltype(f(vector[0]))> {
//     using T    = decltype(f(*vector.begin()));

//     for (const auto& element : vector) {
//         results.push_back(f(element));
//     }

//     return results;
// }

// returns an std::span allocated from alloca
#define ALLOCA_ARR(T, N) std::span(reinterpret_cast<T*>(alloca(sizeof(T) * N)), static_cast<usize>(N))

#define ALLOCA_ARR_WITH_VALUE(T, N, value) ({                                                  \
    auto span = std::span(reinterpret_cast<T*>(alloca(sizeof(T) * N)), static_cast<usize>(N)); \
    for (auto& item : span) {                                                                  \
        item = value;                                                                          \
    }                                                                                          \
    span;                                                                                      \
})

// returns an std::span allocated from alloca
// WARNING IT DOESN'T CALL DESTRUCTOR
#define MAP_VEC_ALLOCA(vector, f...) ({                                   \
    using T    = decltype(f(*vector.begin()));                            \
    T* results = reinterpret_cast<T*>(alloca(sizeof(T) * vector.size())); \
    T* it      = results;                                                 \
    for (auto& element : vector) {                                        \
        *(it++) = f(element);                                             \
    }                                                                     \
    std::span(results, it);                                               \
})



#define RELATIVE_PATH(path) relative_path_impl(__FILE__, path)

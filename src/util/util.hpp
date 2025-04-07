#pragma once

#include <deque>
#include <unordered_map>
#ifndef _WIN32
#include <alloca.h>
#else
#include <malloc.h>
#ifndef alloca
#define alloca(x) _alloca(x)
#endif

#endif

#include <cassert>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "../common.hpp"
#include "../fwd.hpp"

#include "arena_alloc.hpp"

#include "slim_vec.hpp"

#include <cstdio>
#include <stdexcept> // IWYU pragma: export

namespace std {
namespace filesystem {

}
} // namespace std

namespace vke {
namespace fs = std::filesystem;

class ArenaAllocator;

constexpr u32 calculate_dispatch_size(u32 x, u32 subgroup_size) {
    return (x + (subgroup_size - 1)) / subgroup_size;
}

void name_thread(std::thread& thread, const char* name);

auto map_vec(auto&& vector, auto&& f) {
    std::vector<decltype(f(*vector.begin()))> results;
    if constexpr (requires { vector.size(); }) {
        results.reserve(vector.size());
    }
    for (const auto& element : vector) {
        results.push_back(f(element));
    }

    return results;
}

template <size_t small_vec_size = 8>
auto map_vec2small_vec(auto&& vector, auto&& f) {
    vke::SmallVec<decltype(f(*vector.begin())), small_vec_size> results;
    if constexpr (requires { vector.size(); }) {
        results.reserve(vector.size());
    }
    for (const auto& element : vector) {
        results.push_back(f(element));
    }

    return results;
}

auto map_vec_indicies(auto&& vector, auto&& f) {
    std::vector<decltype(f(*vector.begin(), 0))> results;
    if constexpr (requires { vector.size(); }) {
        results.reserve(vector.size());
    }
    int i = 0;
    for (const auto& element : vector) {
        results.push_back(f(element, i));
        i++;
    }

    return results;
}

template <class T>
auto map_vec_into_span(auto&& vector, auto&& f, std::span<T> span) {
    int i = 0;
    for (auto& element : vector) {
        if (i >= span.size()) break;

        span[i] = f(element);
        i++;
    }

    return span;
}

template <typename T>
auto map_optional(const std::optional<T>& opt, auto&& func) -> std::optional<decltype(func(*opt))> {
    if (opt) {
        return std::make_optional(func(*opt));
    }
    return std::nullopt;
}

// https://stackoverflow.com/questions/2067988/how-to-make-a-recursive-lambda
template <class F>
struct YCombinator {
    F f; // the lambda will be stored here

    // a forwarding operator():
    template <class... Args>
    decltype(auto) operator()(Args&&... args) const {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }

    template <class... Args>
    decltype(auto) operator()(Args&&... args) {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }
};

// helper function that deduces the type of the lambda:
template <class F>
YCombinator<std::decay_t<F>> make_y_combinator(F&& f) {
    return {std::forward<F>(f)};
}

std::vector<u8> read_file_binary(const char* name);
std::span<u8> read_file_binary(vke::ArenaAllocator* arena, const char* name);

std::string read_file(const char* name);
std::string_view read_file(vke::ArenaAllocator* arena, const char* name);

std::span<u32> cast_u8_to_span_u32(std::span<u8> span);

template <class ToType, class FromType>
std::span<ToType> span_cast(std::span<FromType> span) {
    return std::span<ToType>(reinterpret_cast<ToType*>(span.data()), span.size_bytes() / sizeof(ToType));
}

template <class ToType, class FromType>
std::span<const ToType> span_cast(std::span<const FromType> span) {
    return std::span<const ToType>(reinterpret_cast<const ToType*>(span.data()), span.size_bytes() / sizeof(ToType));
}

std::string relative_path_impl(const char* source_path, const char* path);

void trace_stack(FILE* stream = stderr);

template <class T>
T round_up_to_multiple(const T& value, const T& multiple) {
    return ((value + (multiple - 1)) / multiple) * multiple;
}

template <class To, class From>
To checked_integer_cast(From n) {
    if constexpr (std::numeric_limits<From>::max() > std::numeric_limits<To>::max()) {
        assert(n <= std::numeric_limits<To>::max());
    }

    if constexpr (std::numeric_limits<From>::min() < std::numeric_limits<To>::min()) {
        assert(n >= std::numeric_limits<To>::min());
    }

    return static_cast<To>(n);
}

template <class T, size_t N>
using CArray = T[N];

template <class T, size_t N>
constexpr size_t array_len(const T (&array)[N]) { return N; }

template <class T, size_t N>
void set_array(CArray<T, N>& array, auto&& function) {
    for (int i = 0; i < N; i++) {
        if constexpr (requires { function(i); }) {
            array[i] = function(i);
        } else {
            array[i] = function();
        }
    }
}

template <class T>
T fold(auto&& container, T initial, auto&& function) {
    for (auto& e : container) {
        initial = function(initial, e);
    }
    return initial;
}

template <class K, class T>
std::optional<T> at(const std::unordered_map<K, T>& map, const K& key) {
    auto it = map.find(key);
    if (it != map.end()) {
        return std::make_optional(it->second);
    } else {
        return std::nullopt;
    }
}

template <class K, class T>
const T* at_ptr(const std::unordered_map<K, T>& map, const K& key) {
    auto it = map.find(key);
    if (it != map.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

template <class K, class T>
T* at_ptr(std::unordered_map<K, T>& map, const K& key) {
    auto it = map.find(key);
    if (it != map.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

template <class T>
std::optional<T> try_pop_front(std::deque<T>& deque) {
    if (deque.empty()) return std::nullopt;

    auto front = std::move(deque.front());
    deque.pop_front();
    return front;
}

template<class T>
std::vector<T> merge_into_vector(auto&& a,auto&& b){
    std::vector<T> out;
    out.reserve(a.size() + b.size());
    out.insert(out.end(),a.begin(),a.end());
    out.insert(out.end(),b.begin(),b.end());
    return out;
}

} // namespace vke

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
// #define MAP_VEC_ALLOCA(vector, ...) [&]{                                  \
//     auto&& f = __VA_ARGS__;\
//     using T    = decltype(f(*vector.begin()));                            \
//     T* results = reinterpret_cast<T*>(alloca(sizeof(T) * vector.size())); \
//     T* it      = results;                                                 \
//     for (auto& element : vector) {                                        \
//         *(it++) = f (element);                                             \
//     }                                                                     \
//     return std::span(results, it);                                               \
// }()

#define MAP_VEC_ALLOCA(vector, ...) vke::map_vec2small_vec(vector, __VA_ARGS__)

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define RELATIVE_PATH(path) relative_path_impl(__FILE__, path)

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define HELPER_MACRO(x) x

#ifndef _WIN32
#define LOG_INFO(fmt, ...) printf(ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_RESET "[INFO] " fmt "\n" __VA_OPT__(, ) __VA_ARGS__)

#define LOG_ERROR(fmt, ...) fprintf(stderr, ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "][%s]" ANSI_COLOR_RED "[ERROR] " fmt ANSI_COLOR_RESET "\n", __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING(fmt, ...) fprintf(stderr, ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "][%s]" ANSI_COLOR_YELLOW "[WARNING] " fmt ANSI_COLOR_RESET "\n", __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_INFO(...)                                                                                             \
    printf(ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_RESET "[INFO] " __VA_ARGS__); \
    printf("\n");

#define LOG_ERROR(fmt, ...)                                                                                               \
    fprintf(stderr, ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_RED "[ERROR] " __VA_ARGS__); \
    fprintf(stderr, ANSI_COLOR_RESET "\n ", );

#define LOG_WARNING(fmt, ...)                                                                                                  \
    fprintf(stderr, ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_YELLOW "[WARNING] " __VA_ARGS__); \
    fprintf(stderr, ANSI_COLOR_RESET "\n");

#endif // !_WIN32

#define THROW_ERROR(fmt, ...)                                                                                                                 \
    {                                                                                                                                         \
        char error_char_buf[1024];                                                                                                            \
        snprintf(error_char_buf, 1024, "[C++][" __FILE__ ":" TOSTRING(__LINE__) "][%s]" fmt, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__); \
        throw std::runtime_error(error_char_buf);                                                                                             \
    }

#define THROW_ERROR_TYPE(T, fmt, ...)                                                                                                         \
    {                                                                                                                                         \
        char error_char_buf[1024];                                                                                                            \
        snprintf(error_char_buf, 1024, "[C++][" __FILE__ ":" TOSTRING(__LINE__) "][%s]" fmt, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__); \
        throw T(error_char_buf);                                                                                                              \
    }

#define VKE_ASSERT(condition) \
    if (!(condition)) {       \
        vke::trace_stack();   \
        assert(condition);    \
    }

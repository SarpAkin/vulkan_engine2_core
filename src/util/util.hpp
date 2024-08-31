#pragma once

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
#include <vector>

#include "../common.hpp"
#include "../fwd.hpp"

#include "arena_alloc.hpp"

#include <cstdio>
#include <stdexcept> // IWYU pragma: export

namespace std {
namespace filesystem {

}
}

namespace vke {
namespace fs = std::filesystem; 

class ArenaAllocator;

auto map_vec(auto&& vector, auto&& f) {
    std::vector<decltype(f(*vector.begin()))> results;
    results.reserve(vector.size());
    for (const auto& element : vector) {
        results.push_back(f(element));
    }

    return results;
}

auto map_vec_indicies(auto&& vector, auto&& f) {
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
#define MAP_VEC_ALLOCA(vector, ...) [&]{                                  \
    auto&& f = __VA_ARGS__;\
    using T    = decltype(f(*vector.begin()));                            \
    T* results = reinterpret_cast<T*>(alloca(sizeof(T) * vector.size())); \
    T* it      = results;                                                 \
    for (auto& element : vector) {                                        \
        *(it++) = f (element);                                             \
    }                                                                     \
    return std::span(results, it);                                               \
}()

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
#define LOG_INFO(...) printf(ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_RESET "[INFO] "  __VA_ARGS__);printf("\n");

#define LOG_ERROR(fmt, ...)                                                                                                   \
    fprintf(stderr, ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_RED "[ERROR] " __VA_ARGS__); \
    fprintf(stderr ,ANSI_COLOR_RESET "\n ", );


#define LOG_WARNING(fmt, ...)                                                                                          \
    fprintf(stderr, ANSI_COLOR_GREEN "[C++][" __FILE__ ":" TOSTRING(__LINE__) "]" ANSI_COLOR_YELLOW "[WARNING] " __VA_ARGS__); \
    fprintf(stderr,ANSI_COLOR_RESET "\n");

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
        trace_stack();        \
        assert(condition);        \
    }

void trace_stack();
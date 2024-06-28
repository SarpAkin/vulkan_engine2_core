#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <vector>

#include "arena_alloc.hpp"

std::vector<u8> read_file_binary(const char* name) {
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<u8> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return buffer;
}

std::span<u32> cast_u8_to_span_u32(std::span<u8> span) {
    if (span.size() % sizeof(u32) != 0) {
        throw std::runtime_error("cast_u8_to_span_u32: span size is not multiple of sizeof(u32)");
    }

    return std::span<u32>(reinterpret_cast<u32*>(span.data()), span.size() / sizeof(u32));
}

std::string read_file(const char* name) {
    std::ifstream file(name);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + std::string(name));
    }

    return std::string(std::istreambuf_iterator<char>(file), {});
}

std::string relative_path_impl(const char* source_path, const char* path) {
    std::filesystem::path p = source_path;
    return p.parent_path() / path;
}

std::span<u8> read_file_binary(vke::ArenaAllocator* arena, const char* name) {
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    u8* data        = arena->alloc<u8>(fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(data), fileSize);
    return std::span(data, fileSize);
}

std::string_view read_file(vke::ArenaAllocator* arena, const char* name) {
    std::ifstream file(name, std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t filesize = static_cast<size_t>(file.tellg());
    char* data      = arena->alloc<char>(filesize + 1);
    data[filesize] = '\0';

    file.seekg(0);
    file.read(reinterpret_cast<char*>(data), filesize);
    return std::string_view(data, filesize);
}

#pragma once

#include <functional>
#include <string_view>
#include <vector>

#include <vke/fwd.hpp>


namespace vke {

namespace spirv_util {

// directly appends a string to the end of the spv without a size
// returns the word count
u32 append_string2spv(std::vector<u32>& spirv, std::string_view str);

// func should be a bool(u32 op,u32 wc,std::span<const u32> op_words)
// should return true to continue evaluation of continuous words
void iterate_spv_words(std::span<const u32> spv, auto&& func) {
    u32 cursor = 5; // skip the header
    while (cursor < spv.size()) {
        u32 word = spv[cursor];
        u32 wc   = word >> 16;
        u32 op   = word & 0xFF'FF;

        // check for invalid word count
        if (wc == 0) throw std::runtime_error("invalid spriv code");

        // break if the function returns false
        if (!func(op, wc, spv.subspan(cursor + 1, wc - 1))) break;

        cursor += wc;
    }
}

void iterate_spv_extensions(std::span<const u32> spv, std::function<bool(std::string_view)> func);

bool has_spv_extension(std::span<const u32> spv, std::string_view extension);

void insert_extension(std::vector<u32>& spv, std::string_view extension);

void append_metadata(std::vector<u32>& spv, std::string_view key, std::span<const u8> data);

} // namespace spirv_util

} // namespace vke
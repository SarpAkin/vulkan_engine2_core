#include "spv_utils.hpp"

#include <cstring>
#include <spirv_reflect.h>

namespace vke {

namespace spirv_util {

u32 append_string2spv(std::vector<u32>& spirv, std::string_view str) {
    u32 wc = (str.size() + 3 + 1 /*1 extra for the null terminator*/) / 4;

    u32 old_size = spirv.size();
    spirv.resize(old_size + wc, 0);
    memcpy(&spirv[old_size], str.data(), str.size());

    return wc;
}
void iterate_spv_extensions(std::span<const u32> spv, std::function<bool(std::string_view)> func) {
    iterate_spv_words(spv, [&](u32 op, u32 wc, std::span<const u32> op_words) {
        // return false to stop the iteration
        if (op != SpvOpExtension) return false;

        const char* str = reinterpret_cast<const char*>(op_words.data());
        return func(std::string_view(str, strnlen(str, op_words.size_bytes())));
    });
}
bool has_spv_extension(std::span<const u32> spv, std::string_view extension) {
    bool flag = false;

    iterate_spv_extensions(spv, [&](std::string_view ext) {
        if (ext == extension) {
            flag = true;
            return false;
        } else {
            return true;
        }
    });

    return flag;
}
void insert_extension(std::vector<u32>& spv, std::string_view extension) {
    u32 insertion_point = 5; // insert right after header

    std::vector<u32> vec = {0};

    u32 wc = append_string2spv(vec, extension);
    vec[0] = SpvOpExtension | ((wc + 1) << 16);

    spv.insert(spv.begin() + insertion_point, vec.begin(), vec.end());
}

void append_metadata(std::vector<u32>& spv, std::string_view key, std::span<const u8> data) {
    const char* ext = "";
    // check if we have the ext
    if (!has_spv_extension(spv, ext)) {
        insert_extension(spv, ext);
    }

    u32 metadata_begin = spv.size();
    spv.push_back(0); // reserve word for op header
    spv.push_back(0); // reserve word for key and data sizes

    // push the string view
    u32 string_view_wc = append_string2spv(spv, key);

    u32 data_wc    = (data.size_bytes() + 3) / 4;
    u32 data_begin = spv.size();
    spv.resize(spv.size() + data_wc);

    memcpy(&spv[data_begin], data.data(), data.size_bytes());

    assert(string_view_wc < 0x1'0000);
    assert(data_wc < 0x1'0000);

    spv[metadata_begin + 0] = SpvOpExtInst | (spv.size() - metadata_begin);
    spv[metadata_begin + 1] = (string_view_wc & 0xFFFF) | (data_wc << 16);
}
} // namespace spirv_util
} // namespace vke
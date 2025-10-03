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

void spv_change_set_numbers(std::vector<u32>& spv, int target_set, int new_value) {
    iterate_spv_words(spv, [&](u32 op, u32 wc, std::span<u32> words) {
        if (op != SpvOpDecorate) return true;

        // decorate
        //  words
        //  0        1               2
        //  ID?      decoration_type value

        // 0x22 means it is a set decoration
        if (words[1] == 0x22 && words[2] == target_set) {
            words[2] = new_value;
        }

        // words[1] == 0x21 for binding

        return true;
    });
}

std::unordered_set<u32> spv_find_set_numbers(std::span<const u32> spv) {
    std::unordered_set<u32> numbers;

    iterate_spv_words(spv, [&](u32 op, u32 wc, std::span<const u32> words) {
        if (op != SpvOpDecorate) return true;

        if (words[1] == 0x22) {
            numbers.emplace(words[2]);
        }

        return true;
    });

    return numbers;
}

void spv_iterate_descriptor_bindings(std::span<u32> spv, std::function<void(u32& /*set*/, u32& /*binding*/)> f) {

    u32 *set_data = nullptr, *binding_data = nullptr;
    u32 id = 0xFFFF'FFFF;

    iterate_spv_words(spv, [&](u32 op, u32 wc, std::span<u32> words) {
        if (op != SpvOpDecorate) return true;

        if (words[0] != id) {
            set_data     = nullptr;
            binding_data = nullptr;
            id           = words[0];
        }

        //0x22 for descriptor sets, 0x21 for descriptor bindings
        if (words[1] == 0x22) {
            set_data = words.data();
        } else if (words[1] == 0x21) {
            binding_data = words.data();
        }

        // if we have bot call the function
        if (set_data && binding_data) {
            // function can change the values
            f(set_data[2], binding_data[2]);
        }

        return true;
    });
}

} // namespace spirv_util
} // namespace vke
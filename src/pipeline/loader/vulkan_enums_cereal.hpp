#pragma once

#include <vulkan/vulkan.h>

#include <string>

#include <cereal/cereal.hpp>

// Utility functions for serializing enums to string
template <class Archive>
void serialize(Archive& archive, VkPolygonMode& mode) {
    std::string str;
    if (Archive::is_saving::value) {
        switch (mode) {
        case VK_POLYGON_MODE_FILL:
            str = "FILL";
            break;
        case VK_POLYGON_MODE_LINE:
            str = "LINE";
            break;
        case VK_POLYGON_MODE_POINT:
            str = "POINT";
            break;
        default:
            break;
        }
        archive(str);
    } else {
        archive(str);

        if (str == "FILL") {
            mode = VK_POLYGON_MODE_FILL;
        } else if (str == "LINE") {
            mode = VK_POLYGON_MODE_LINE;
        } else if (str == "POINT") {
            mode = VK_POLYGON_MODE_POINT;
        }
    }
}

template <class Archive>
void serialize(Archive& archive, VkPrimitiveTopology& mode) {
    std::string str;
    if (Archive::is_saving::value) {
        switch (mode) {
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
            str = "POINT_LIST";
            break;
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
            str = "LINE_LIST";
            break;
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            str = "LINE_STRIP";
            break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            str = "TRIANGLE_LIST";
            break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            str = "TRIANGLE_STRIP";
            break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            str = "TRIANGLE_FAN";
            break;
        default:    
            break;
        }
        archive(str);    
    } else {
        archive(str);

        if (str == "POINT_LIST") {
            mode = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        } else if (str == "LINE_LIST") {
            mode = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        } else if (str == "LINE_STRIP") {
            mode = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        } else if (str == "TRIANGLE_LIST") {
            mode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        } else if (str == "TRIANGLE_STRIP") {
            mode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        } else if (str == "TRIANGLE_FAN") {
            mode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        }
    }
}

template <class Archive>
void serialize(Archive& archive, VkCullModeFlagBits& mode) {
    std::string str;
    if (Archive::is_saving::value) {
        switch (mode) {
        case VK_CULL_MODE_NONE:
            str = "NONE";
            break;
        case VK_CULL_MODE_FRONT_BIT:
            str = "FRONT";
            break;
        case VK_CULL_MODE_BACK_BIT:
            str = "BACK";
            break;
        case VK_CULL_MODE_FRONT_AND_BACK:
            str = "FRONT_AND_BACK";
            break;
        default:
            str = "NONE";
            break;
        }
        archive(str);
    } else {
        archive(str);

        if (str == "NONE") {
            mode = VK_CULL_MODE_NONE;
        } else if (str == "FRONT") {
            mode = VK_CULL_MODE_FRONT_BIT;
        } else if (str == "BACK") {
            mode = VK_CULL_MODE_BACK_BIT;
        } else if (str == "FRONT_AND_BACK") {
            mode = VK_CULL_MODE_FRONT_AND_BACK;
        }
    }
}

#pragma once

#include <filesystem>
#include <optional>
#include <span>

#include <vke/fwd.hpp>
#include <vulkan/vulkan.h>

namespace vke {

class IGlslIncludeResolver {
public:
    struct IncludeResolveParameters {
        const fs::path& requested_source;
        const fs::path& requesting_source;
        bool is_relative;
    };

    struct IncludeResolverReturn{
        std::string_view content;
        std::string_view path;
    };

    virtual std::optional<IncludeResolverReturn> resolve_include(ArenaAllocator* arena_alloc, const IncludeResolveParameters& parameters) = 0;

    ~IGlslIncludeResolver(){}
};

} // namespace vke

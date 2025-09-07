#include "library_include_resolver.hpp"

#include <vke/util.hpp>

namespace vke {
std::optional<IGlslIncludeResolver::IncludeResolverReturn> LibraryIncludeResolver::resolve_include(ArenaAllocator* arena_alloc, const IncludeResolveParameters& parameters) {
    if (!parameters.is_relative) return std::nullopt;

    auto path = m_base_path / parameters.requested_source;

    if (!fs::exists(path)) return std::nullopt;

    return IncludeResolverReturn{
        .content = read_file(arena_alloc, path.c_str()),
        .path = arena_alloc->create_str_copy(path.c_str()),
    };
}

} // namespace vke

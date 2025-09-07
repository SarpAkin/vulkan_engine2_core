#pragma once

#include "iinclude_resolver.hpp"

namespace vke {

class LibraryIncludeResolver : public IGlslIncludeResolver {
public:
    LibraryIncludeResolver(std::string_view dir) { m_base_path = dir; }
    ~LibraryIncludeResolver() {}

    std::optional<IncludeResolverReturn> resolve_include(ArenaAllocator* arena_alloc, const IncludeResolveParameters& parameters) override;

private:
    fs::path m_base_path;
};

} // namespace vke
#pragma once

#include "iinclude_resolver.hpp"

namespace vke {

class RelativeIncludeResolver : public IGlslIncludeResolver {
public:
    ~RelativeIncludeResolver(){}

    std::optional<IncludeResolverReturn> resolve_include(ArenaAllocator* arena_alloc, const IncludeResolveParameters& parameters) override;
private:
};

} // namespace vke
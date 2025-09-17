#pragma once

#include <memory>

#include <vke/fwd.hpp>

#include "pipeline_globals_provider.hpp"

namespace vke {
class IPipelineLoader {
public:
    struct DebugLoaderArguments {
        std::vector<std::string> pipeline_search_paths;
        std::vector<std::string> shader_lib_paths; // paths for builtin shaders like "@default.vert"
        bool reloadable;
    };

    virtual ~IPipelineLoader() = default;

    virtual std::unique_ptr<IPipeline> load(const char* pipeline_name)                                    = 0;
    virtual void set_pipeline_globals_provider(std::shared_ptr<PipelineGlobalsProvider> globals_provider) = 0;
    virtual PipelineGlobalsProvider* get_pipeline_globals_provider()                                      = 0;

    static std::unique_ptr<IPipelineLoader> make_debug_loader(const DebugLoaderArguments& args);
};

} // namespace vke
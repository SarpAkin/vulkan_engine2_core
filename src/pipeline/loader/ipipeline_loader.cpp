#include "ipipeline_loader.hpp"

#include "debug_loader.hpp"
#include "hot_reload/hot_reloader.hpp"
#include "pipeline_globals_provider.hpp"

namespace vke {

std::unique_ptr<IPipelineLoader> IPipelineLoader::make_debug_loader(const DebugLoaderArguments& args) {
    if (args.reloadable) {
        return std::make_unique<ReloadableLoader>(args);
    } else {
        return std::make_unique<DebugPipelineLoader>(args);
    }
}

} // namespace vke
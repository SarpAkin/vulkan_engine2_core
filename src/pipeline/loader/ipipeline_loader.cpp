#include "ipipeline_loader.hpp"

#include "pipeline_globals_provider.hpp"
#include "debug_loader.hpp"
#include "hot_reload/hot_reloader.hpp"

namespace vke {

std::unique_ptr<IPipelineLoader> IPipelineLoader::make_debug_loader(const char* pipeline_search_path, bool reloadable) {
    if(reloadable){
        return std::make_unique<ReloadableLoader>(pipeline_search_path);
    }else{
        return std::make_unique<DebugPipelineLoader>(pipeline_search_path);
    }
}


} // namespace vke
#pragma once

#include <memory>

#include <vke/fwd.hpp>

namespace vke  {
    class IPipelineLoader {
    public:
        virtual ~IPipelineLoader() = default;

        virtual std::unique_ptr<IPipeline> load(const char* pipeline_name) = 0;
        virtual void set_pipeline_globals_provider(std::unique_ptr<class PipelineGlobalsProvider> globals_provider) = 0;
        virtual PipelineGlobalsProvider* get_pipeline_globals_provider() = 0;
    };

}
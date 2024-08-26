#include "ipipeline.hpp"

#include "../commandbuffer.hpp"

namespace vke {

void IPipeline::bind(CommandBuffer& cmd) {
    vkCmdBindPipeline(cmd.handle(), bind_point(), handle());

}
} // namespace vke
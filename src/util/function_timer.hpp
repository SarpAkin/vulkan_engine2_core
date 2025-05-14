#pragma once

#include <chrono>

namespace vke {
namespace impl {

class FunctionTimer {
public:
    FunctionTimer(const char* func_name);
    ~FunctionTimer();

private:
    std::chrono::steady_clock::time_point m_start;
    const char* m_func_name;
};

} // namespace impl

} // namespace vke

#define BENCHMARK_FUNCTION() vke::impl::FunctionTimer ______function_timer(__PRETTY_FUNCTION__);

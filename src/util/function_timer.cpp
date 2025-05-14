#include "function_timer.hpp"

#include <deque>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#include "util.hpp"

namespace vke {
namespace impl {

struct FunctionStats {
    double total_time         = 0.0;
    uint64_t invocation_count = 0;
    std::deque<double> times;
};

class GlobalTimingManager {
public:
    void add_records(std::unordered_map<const char*, FunctionStats>&& stats) {
        std::lock_guard<std::mutex> guard(m_lock);

        for (auto& [fname, new_stats] : stats) {
            auto& global_stats = m_stats[fname];
            global_stats.total_time += new_stats.total_time;
            global_stats.invocation_count += new_stats.invocation_count;

            // Move times into global stats
            global_stats.times.insert(
                global_stats.times.end(),
                std::make_move_iterator(new_stats.times.begin()),
                std::make_move_iterator(new_stats.times.end()));
        }
    }

    ~GlobalTimingManager() {

        fs::path path = ".misc/function_times";
        if(!fs::is_directory(path)){
            fs::create_directory(path);
        }


        for (auto& [fname, stats] : m_stats) {
            double avg_time = stats.total_time / stats.invocation_count;

            if (avg_time < 0.01) {
                printf("[Function Timer] function: %s took %.4f µs on avarage to run. invocation count = %ld \n", fname, avg_time * 1E3, stats.invocation_count);
            } else {
                printf("[Function Timer] function: %s took %.4f ms on avarage to run. invocation count = %ld \n", fname, avg_time, stats.invocation_count);
            }

            auto file = fopen((path / fname).c_str(), "w");
            for (auto time : stats.times) {
                fprintf(file, "%.4lf\n", time);
            }
            fclose(file);
        }
    }

private:
    std::mutex m_lock;
    std::unordered_map<const char*, FunctionStats> m_stats;
};

static GlobalTimingManager global_man;

class TimingManager {
public:
    void function_time(const char* function, double time) {

        auto& stats = m_stats[function];
        stats.total_time += time;
        stats.invocation_count += 1;
        stats.times.push_back(time);
    }

    ~TimingManager() {
        global_man.add_records(std::move(m_stats));
    }

private:
    std::unordered_map<const char*, FunctionStats> m_stats;
};

static thread_local TimingManager man;

FunctionTimer::FunctionTimer(const char* func_name) {
    m_start     = std::chrono::steady_clock::now();
    m_func_name = func_name;
}

FunctionTimer ::~FunctionTimer() {
    double elapsed_time = ((double)(std::chrono::steady_clock::now() - m_start).count()) / 1000000.0;
    man.function_time(m_func_name, elapsed_time);
    // fmt::print("[Function Timer] function: {} took {} ms to run\n", m_func_name, elapsed_time);
}
} // namespace impl

} // namespace vke

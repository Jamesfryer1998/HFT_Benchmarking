#pragma once

#include <chrono>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace hft {

// RDTSC-based timing for ultra-low latency measurement
// Only available on x86/x86_64 architectures
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #define HFT_HAS_RDTSC 1

    inline uint64_t rdtsc() noexcept {
        uint32_t lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
        return (static_cast<uint64_t>(hi) << 32) | lo;
    }
#else
    #define HFT_HAS_RDTSC 0
#endif

// High-resolution timer using chrono
class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    Timer() noexcept : start_(Clock::now()) {}

    void reset() noexcept { start_ = Clock::now(); }

    [[nodiscard]] uint64_t elapsed_ns() const noexcept {
        auto end = Clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
    }

    [[nodiscard]] static uint64_t now_ns() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()).count();
    }

private:
    TimePoint start_;
};

// Latency statistics tracker
class LatencyTracker {
public:
    explicit LatencyTracker(size_t reserve_samples = 1'000'000)
        : samples_()
    {
        samples_.reserve(reserve_samples);
    }

    void record(uint64_t latency_ns) noexcept {
        samples_.push_back(latency_ns);
    }

    void clear() noexcept {
        samples_.clear();
    }

    [[nodiscard]] size_t count() const noexcept {
        return samples_.size();
    }

    [[nodiscard]] uint64_t min() const noexcept {
        if (samples_.empty()) return 0;
        return *std::min_element(samples_.begin(), samples_.end());
    }

    [[nodiscard]] uint64_t max() const noexcept {
        if (samples_.empty()) return 0;
        return *std::max_element(samples_.begin(), samples_.end());
    }

    [[nodiscard]] uint64_t mean() const noexcept {
        if (samples_.empty()) return 0;
        uint64_t sum = 0;
        for (auto sample : samples_) {
            sum += sample;
        }
        return sum / samples_.size();
    }

    [[nodiscard]] uint64_t percentile(double p) const noexcept {
        if (samples_.empty()) return 0;

        auto sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        size_t index = static_cast<size_t>(p * sorted.size());
        if (index >= sorted.size()) {
            index = sorted.size() - 1;
        }

        return sorted[index];
    }

    [[nodiscard]] uint64_t p50() const noexcept { return percentile(0.50); }
    [[nodiscard]] uint64_t p95() const noexcept { return percentile(0.95); }
    [[nodiscard]] uint64_t p99() const noexcept { return percentile(0.99); }

private:
    std::vector<uint64_t> samples_;
};

// RAII timer that automatically records latency
class ScopedLatencyTimer {
public:
    explicit ScopedLatencyTimer(LatencyTracker& tracker) noexcept
        : tracker_(tracker)
        , timer_()
    {}

    ~ScopedLatencyTimer() {
        tracker_.record(timer_.elapsed_ns());
    }

private:
    LatencyTracker& tracker_;
    Timer timer_;
};

} // namespace hft

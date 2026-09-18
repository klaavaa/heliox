#pragma once
#include <chrono>
#include <functional>
#include <print>

template <typename Function, typename... Args>
    requires std::invocable<Function, Args...>
double timeit(Function&& func, Args&& ... elements)
{
	auto start = std::chrono::high_resolution_clock::now();
    std::invoke(func, elements...);
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	return static_cast<double>(milliseconds);
}

static auto PERF_MILLI_COUNT = std::chrono::high_resolution_clock::now(); 

inline void HX_PERF_START() {
	PERF_MILLI_COUNT = std::chrono::high_resolution_clock::now();
}

inline void HX_PERF_END(std::string_view info) {
	auto elapsed = std::chrono::high_resolution_clock::now() - PERF_MILLI_COUNT;
	double milliseconds = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    std::println("{}: {:.0f}ms", info, milliseconds);
}



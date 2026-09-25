#pragma once
#include <chrono>
#include <functional>
#include <print>
#include <stack>
#include <source_location>

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
static auto PERF_STACK = std::stack<std::chrono::high_resolution_clock::time_point>(); 

inline void HX_PERF_START() {
    PERF_STACK.push(std::chrono::high_resolution_clock::now());
}

inline void HX_PERF_END(std::string_view info, std::source_location location = std::source_location::current()) {
    if (PERF_STACK.empty()) 
    {
        std::println("HX_PERF_END called without HX_PERF_START\nat: {}: {}", location.file_name(), location.line());
    }
	auto elapsed = std::chrono::high_resolution_clock::now() - PERF_STACK.top();
    PERF_STACK.pop();
	double milliseconds = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    std::println("{}: {:.0f}ms", info, milliseconds);
}



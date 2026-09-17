#pragma once
#include <chrono>
#include <functional>

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





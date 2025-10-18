#pragma once
#include <chrono>
#include <functional>
#include <initializer_list>


template <typename ret, typename... param>
using fn = std::function<ret(param...)>;

template <typename ret, typename... param>
double timeit(fn<ret, param...> func, param&... elements)
{
	auto start = std::chrono::high_resolution_clock::now();
	func(elements...);
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
	return static_cast<double>(microseconds) * 0.000001;

}





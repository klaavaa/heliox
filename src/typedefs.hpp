#pragma once
#include <memory>

namespace hx
{

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

template<typename T>
using uptr = std::unique_ptr<T>;

template<typename T>
using sptr = std::shared_ptr<T>;
}


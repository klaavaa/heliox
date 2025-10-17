#pragma once
#include <memory>

namespace hx
{

template<typename T>
using uptr = std::unique_ptr<T>;

template<typename T>
using sptr = std::shared_ptr<T>;
}


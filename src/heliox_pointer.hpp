#pragma once
#include <memory>

template<typename T>
using hx_uptr = std::unique_ptr<T>;

template<typename T>
using hx_sptr = std::shared_ptr<T>;

using std::make_shared;
using std::make_unique;
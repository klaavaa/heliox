#pragma once
#include <vector>
#include "heliox_function.hpp"
#include "heliox_pointer.hpp"
namespace hx
{
    struct program 
    {
        program(std::vector<uptr<function>> functions)
            : functions(std::move(functions)) {}

        std::vector<uptr<function>> functions;
    };

}


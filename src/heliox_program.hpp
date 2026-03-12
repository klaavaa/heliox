#pragma once
#include <vector>
#include "heliox_function.hpp"
#include "heliox_pointer.hpp"
namespace hx
{
    struct Program 
    {
        Program(std::vector<uptr<function>> functions)
            : functions(std::move(functions)) {}

        std::vector<uptr<function>> functions;
    };

}


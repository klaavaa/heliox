
#pragma once
#include <vector>
#include <unordered_set>
#include "heliox_function.hpp"
#include "heliox_pointer.hpp"

namespace hx
{
    struct Program 
    {
        Program(std::vector<uptr<function>> functions, std::unordered_set<std::string> modules,
                std::unordered_set<std::string> imports)
            : functions(std::move(functions)),
              modules(modules),
              imports(std::move(imports)){}

        std::vector<uptr<function>> functions;
        std::unordered_set<std::string> modules;
        std::unordered_set<std::string> imports;
    };

}


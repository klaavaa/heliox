#pragma once
#include <vector>
#include <format>
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
template<>
struct std::formatter<hx::program> : std::formatter<std::string>
{
    auto format(const hx::program& prog, auto& ctx) const
    {
        std::string formatted = "program::begin\n";
        for (const auto& func : prog.functions)
        {
            formatted += std::format("{}", *func); 
        }
        
        return std::formatter<std::string>::format(formatted, ctx);
    }

};



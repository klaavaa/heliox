#pragma once
#include <format>
#include "heliox_program.hpp"
#include "heliox_statement.hpp"
#include "heliox_function.hpp"

template<>
struct std::formatter<hx::identifier_literal_expr> : std::formatter<std::string>
{
    auto format(const hx::identifier_literal_expr& identifier, auto& ctx) const
    {
        std::string formatted = std::format("{}", identifier.name);
        return std::formatter<std::string>::format(formatted, ctx);
    }

};
/*
template<>
struct std::formatter<hx::type_data> : std::formatter<std::string>
{
    auto format(const hx::type_data& type, auto& ctx) const
    {
        std::string formatted = std::format("{}", type.byte_size);
        return std::formatter<std::string>::format(formatted, ctx);
    }

};

template<>
struct std::formatter<hx::variable_declaration_statement> : std::formatter<std::string>
{
    auto format(const hx::variable_declaration_statement& variable_decl, auto& ctx) const
    {
        std::string formatted = std::format("{} {}", 
                variable_decl.var_type,
                *variable_decl.var_identifier);
        return std::formatter<std::string>::format(formatted, ctx);
    }

};
*/


template <>
struct std::formatter<hx::function> : std::formatter<std::string>
{
    auto format(const hx::function& func, auto& ctx) const
    {
        std::string formatted = std::format("function{}::begin\n(", func.identifier);
        /*
        for (const auto& param: func.params)
        {
            formatted += std::format("{}", *param);
        }    
        formatted +=  std::format("function{}::end\n(", *func.identifier);
        */
        return std::formatter<std::string>::format(formatted, ctx);
    }

};


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



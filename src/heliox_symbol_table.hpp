#pragma once

#include <unordered_map>
#include <vector>
#include "heliox_types.hpp"
#include "heliox_pointer.hpp"
#include "heliox_instructions.hpp"

namespace hx
{


    struct VariableSymbol
    {
        VariableSymbol(type_data type_info, virtual_register vr, bool is_parameter)
            : type_info(type_info), vr(vr), is_parameter(is_parameter) {}

        type_data type_info;
        virtual_register vr;
        bool is_parameter;
    };

    struct FunctionSymbol
    {
        FunctionSymbol(type_data return_type, const std::vector<type_data>& parameter_types, uint32_t id) 
        : parameter_types(parameter_types), return_type(return_type), id(id) {}
        type_data return_type;
        std::vector<type_data> parameter_types;
        uint32_t id;
    };
    


    class SymbolTable
    {
    public:        
        SymbolTable();
        sptr<SymbolTable> add_table();
        void add_variable_symbol(std::string name, type_data type_info, virtual_register vr, bool is_parameter = false);
        void add_function_symbol(std::string name, type_data return_type, const std::vector<type_data>& parameter_types);

        VariableSymbol& find_variable_symbol(const std::string& name);
        FunctionSymbol& find_function_symbol(const std::string& name);
         
        SymbolTable* get_parent();

        uint32_t add_string(std::string value);
        uint32_t add_function(std::string function_name);

        std::string get_string_from_id(uint32_t id);
        
        std::string get_function_name_from_id(uint32_t id);
        FunctionSymbol get_function_symbol_from_id(uint32_t id);
        
        const std::unordered_map<uint32_t, std::string>& get_string_table() const { return string_table; }
        const std::unordered_map<uint32_t, std::string>& get_function_table() const { return function_table; }

        std::vector<virtual_register> get_all_variable_virtual_registers() const
        {
            std::vector<virtual_register> all;
            if (parent)
            {
                const std::vector<virtual_register> parents = parent->get_all_variable_virtual_registers();
                all.insert(all.end(), parents.begin(), parents.end());
            }
            for (auto& [key, symbol] : variable_symbols)
            {
                all.push_back(symbol.vr); 
            }
            return all;
        }

    private:
        std::unordered_map<std::string, VariableSymbol> variable_symbols;
        std::unordered_map<std::string, FunctionSymbol> function_symbols;
        std::vector<sptr<SymbolTable>> child_tables;
        SymbolTable* parent = nullptr;

        std::unordered_map<uint32_t, std::string> string_table;
        std::unordered_map<uint32_t, std::string> function_table;

        uint32_t string_id = 0;
        uint32_t function_id = 0;
    };

}

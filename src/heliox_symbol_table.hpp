#pragma once

#include <unordered_map>
#include <vector>
#include "heliox_types.hpp"
#include "heliox_pointer.hpp"
#include "heliox_instructions.hpp"

namespace hx
{
    enum class SymbolType
    {
        VARIABLE,
        FUNCTION
    };


    struct Symbol
    {
        Symbol(SymbolType sym_type,
                type_data type_info,
                virtual_register vr,
                bool is_parameter)
            : sym_type(sym_type), type_info(type_info), vr(vr), is_parameter(is_parameter) {}

        SymbolType sym_type;
        type_data type_info;
        virtual_register vr;
        bool is_parameter;
    };
    


    class SymbolTable
    {
    public:        
        SymbolTable();
        sptr<SymbolTable> add_table();
        void add_symbol(std::string name, SymbolType sym_type, type_data type_info, virtual_register vr, bool is_parameter = false);
        Symbol find_symbol(const std::string& name, SymbolType sym_type);
         
        SymbolTable* get_parent();

        uint32_t add_string(std::string value);
        uint32_t add_function(std::string function_name);

        uint32_t get_function_id(const std::string& name);
        std::string get_string_from_id(uint32_t id);


    private:
        std::unordered_map<std::string, Symbol> symbols;
        std::vector<sptr<SymbolTable>> child_tables;
        SymbolTable* parent = nullptr;

        std::unordered_map<uint32_t, std::string> string_table;
        std::unordered_map<std::string, uint32_t> function_table;

        uint32_t string_id = 0;
        uint32_t function_id = 0;
    };

}

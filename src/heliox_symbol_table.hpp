#pragma once

#include <unordered_map>
#include <vector>
#include <optional>
#include "heliox_types.hpp"
#include "heliox_pointer.hpp"

namespace hx
{
    enum class symbol_type
    {
        VARIABLE,
        FUNCTION
    };


    struct symbol
    {
        symbol(symbol_type sym_type,
                type_data type_info,
                uint32_t stack_position)
            : sym_type(sym_type), type_info(type_info), stack_position(stack_position) {}

        symbol_type sym_type;
        type_data type_info;
        uint32_t stack_position;
    };

    class symbol_table
    {
    public:        
        symbol_table(uint32_t next_stack_position = 8);
        sptr<symbol_table> add_table(bool is_function_body); 
        void add_symbol(std::string name, symbol_type sym_type, type_data type_info); 
        symbol find_symbol(const std::string& name, symbol_type sym_type);
         
        symbol_table* get_parent();
    private:
        
        uint32_t calculate_stack_position(const type_data& type_info);

    private:
        std::unordered_map<std::string, symbol> symbols;
        std::vector<sptr<symbol_table>> child_tables;
        symbol_table* parent = nullptr;
        uint32_t next_stack_position;
    };

}

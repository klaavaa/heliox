#pragma once

#include <unordered_map>
#include <vector>
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
                int64_t stack_position,
                bool is_param = false)
            : sym_type(sym_type), type_info(type_info), stack_position(stack_position),
              is_param(is_param) {}

        symbol_type sym_type;
        type_data type_info;
        int64_t stack_position;
        bool is_param;
    };
    


    class symbol_table
    {
    public:        
        symbol_table(uint32_t next_stack_position = 8);
        sptr<symbol_table> add_table(bool is_function_body); 
        void add_symbol(std::string name, symbol_type sym_type, type_data type_info, bool is_param=false); 
        symbol find_symbol(const std::string& name, symbol_type sym_type);
         
        symbol_table* get_parent();

        uint32_t add_string(std::string value);
        uint32_t add_function(std::string function_name);

        uint32_t get_function_id(const std::string& name);
        std::string get_string_from_id(uint32_t id);

    private:
        
        int64_t calculate_stack_position(const type_data& type_info);
        int64_t calculate_parameter_stack_position(const type_data& type_info);

    private:
        std::unordered_map<std::string, symbol> symbols;
        std::vector<sptr<symbol_table>> child_tables;
        symbol_table* parent = nullptr;
        int64_t next_stack_position;
        int64_t next_parameter_stack_position;
        uint32_t function_param_count;

        std::unordered_map<uint32_t, std::string> string_table;
        std::unordered_map<std::string, uint32_t> function_table;

        uint32_t string_id = 0;
        uint32_t function_id = 0;
    };

}

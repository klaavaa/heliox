#include "heliox_symbol_table.hpp"
#include <print>

namespace hx
{

symbol_table::symbol_table(uint32_t next_stack_position) 
    : next_stack_position(next_stack_position),
      next_parameter_stack_position(-16),
      function_param_count(0) {}

sptr<symbol_table> symbol_table::add_table(bool is_function_body)
{
    uint32_t new_stack_position = next_stack_position;

    if (is_function_body)
        new_stack_position = 8;

    sptr<symbol_table> new_table = 
        std::make_shared<symbol_table>(new_stack_position);

    new_table->parent = this; 
    
    child_tables.push_back(new_table);
    return new_table;
}

void symbol_table::add_symbol(std::string name, symbol_type sym_type, type_data type_info, bool is_param)
{
    if (symbols.contains(name))            
    {
        //TODO ERROR
        std::println("Symbol {} already defined in current scope", name); 
        exit(-1);
    }
    switch (sym_type)
    {
        case symbol_type::VARIABLE:
        {
            if (is_param)
            {
                if (function_param_count < 6)
                {
                    function_param_count++; 
                    symbols.emplace(name, symbol{sym_type, type_info, function_param_count, is_param});
                    break;
                }
                int64_t parameter_stack_position = calculate_parameter_stack_position(type_info);
                symbols.emplace(name, symbol{sym_type, type_info, parameter_stack_position});
                break; 
                
            }
            int64_t stack_position = calculate_stack_position(type_info);
            symbols.emplace(name, symbol{sym_type, type_info, stack_position});
            break;
        }
        case symbol_type::FUNCTION:
        {
            symbols.emplace(name, symbol{sym_type, type_info, 0});
            add_function(name);
            break;
        }
    }
}

symbol symbol_table::find_symbol(const std::string& name, symbol_type sym_type) 
{

    if (symbols.count(name))
    {
        symbol sym = symbols.at(name);
        if (sym.sym_type == sym_type)
            return sym;
        //TODO ERROR
        std::println("Found symbol '{}' of wrong symbol type", name);
        exit(-1);
    }
    if (parent != nullptr )
    {
        return parent->find_symbol(name, sym_type);
    }
    //TODO ERROR
    std::println("Symbol '{}' not defined in current scope", name);
    exit(-1);
}


int64_t symbol_table::calculate_stack_position(const type_data& type_info)
{
    int64_t stack_position =  next_stack_position; 
    next_stack_position += type_info.byte_size;
    return stack_position;
}

int64_t symbol_table::calculate_parameter_stack_position(const type_data& type_info)
{
   int64_t parameter_stack_position = next_parameter_stack_position;
   next_parameter_stack_position -= type_info.byte_size;
   return parameter_stack_position;
}

symbol_table* symbol_table::get_parent()
{
    return parent;
}

uint32_t symbol_table::add_string(std::string value)
{
    string_table[string_id] = value;
    string_id++;
    return string_id - 1;
}
uint32_t symbol_table::add_function(std::string function_name)
{
    function_table[function_name] = function_id;
    function_id++;
    return function_id - 1;
}

uint32_t symbol_table::get_function_id(const std::string& name)
{
    if (function_table.contains(name))
    {
        return function_table.at(name);
    }
    // TODO ERROR
    std::println("Trying to get function: '{}' id but it's not found", name);
    exit(-1);
}

std::string symbol_table::get_string_from_id(uint32_t id)
{
     if (string_table.contains(id))
    {
        return string_table.at(id);
    }
    // TODO ERROR
    std::println("Trying to get string from id: '{}' but it's not found", id);
    exit(-1);

}

}

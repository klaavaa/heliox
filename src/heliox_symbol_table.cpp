#include "heliox_symbol_table.hpp"
#include <print>

namespace hx
{

symbol_table::symbol_table(uint32_t next_stack_position) 
    : next_stack_position(next_stack_position) {}

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

void symbol_table::add_symbol(std::string name, symbol_type sym_type, type_data type_info)
{
    if (symbols.contains(name))            
    {
        //TODO ERROR
        std::println("Symbol {} already defined in current scope", name); 
        exit(-1);
    }
    
    if (sym_type == symbol_type::VARIABLE)
    {
        uint32_t stack_position = calculate_stack_position(type_info);
        symbols.emplace(name, symbol{sym_type, type_info, stack_position});
    }
    else
    {
        symbols.emplace(name, symbol{sym_type, type_info, 0});
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


uint32_t symbol_table::calculate_stack_position(const type_data& type_info)
{
    uint32_t stack_position =  next_stack_position; 
    next_stack_position += type_info.byte_size;
    return stack_position;
}

symbol_table* symbol_table::get_parent()
{
    return parent;
}

}

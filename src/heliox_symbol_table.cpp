#include "heliox_symbol_table.hpp"
#include <print>

namespace hx
{

SymbolTable::SymbolTable() {}

sptr<SymbolTable> SymbolTable::add_table()
{
    sptr<SymbolTable> new_table = 
        std::make_shared<SymbolTable>();

    new_table->parent = this; 
    
    child_tables.push_back(new_table);
    return new_table;
}

void SymbolTable::add_variable_symbol(std::string name, type_data type_info, virtual_register vr, bool is_parameter)
{
    if (variable_symbols.contains(name))            
    {
        //TODO ERROR
        std::println("Symbol {} already defined in current scope", name); 
        exit(-1);
    }
    variable_symbols.emplace(name, VariableSymbol{type_info, vr, is_parameter});
}
void SymbolTable::add_function_symbol(std::string name, type_data return_type, const std::vector<type_data>& param_types)
{
    uint32_t id = add_function(name);
    function_symbols.emplace(name, FunctionSymbol{return_type, param_types, id});
}

VariableSymbol SymbolTable::find_variable_symbol(const std::string& name) 
{

    if (variable_symbols.count(name))
    {
        VariableSymbol sym = variable_symbols.at(name);
        return sym;
    }
    if (parent != nullptr )
    {
        return parent->find_variable_symbol(name);
    }
    //TODO ERROR
    std::println("VariableSymbol '{}' not defined in current scope", name);
    exit(-1);
}

FunctionSymbol SymbolTable::find_function_symbol(const std::string& name) 
{
    if (parent != nullptr )
    {
        return parent->find_function_symbol(name);
    }
    if (function_symbols.count(name))
    {
        FunctionSymbol sym = function_symbols.at(name);
        return sym;
    }

    //TODO ERROR
    std::println("FunctionSymbol '{}' not defined in current scope", name);
    exit(-1);

}

SymbolTable* SymbolTable::get_parent()
{
    return parent;
}

uint32_t SymbolTable::add_string(std::string value)
{
    string_table[string_id] = value;
    string_id++;
    return string_id - 1;
}
uint32_t SymbolTable::add_function(std::string function_name)
{
    function_table[function_id] = function_name;
    function_id++;
    return function_id - 1;
}

std::string SymbolTable::get_string_from_id(uint32_t id)
{
    if (string_table.contains(id))
    {
        return string_table.at(id);
    }
    // TODO ERROR
    std::println("Trying to get string from id: '{}' but it's not found", id);
    exit(-1);

}
std::string SymbolTable::get_function_name_from_id(uint32_t id)
{
    if (function_table.contains(id))
    {
        return function_table[id];
    }
    std::println("Trying to get function name from id: '{}' but it's not found", id);
    exit(-1);

}
FunctionSymbol SymbolTable::get_function_symbol_from_id(uint32_t id)
{
    if (function_table.contains(id))
    {
        return find_function_symbol(function_table[id]);
    }
    std::println("Trying to get function from id: '{}' but it's not found", id);
    exit(-1);
}
}

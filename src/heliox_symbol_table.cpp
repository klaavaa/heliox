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

void SymbolTable::add_symbol(std::string name, SymbolType sym_type, type_data type_info, virtual_register vr)
{
    if (symbols.contains(name))            
    {
        //TODO ERROR
        std::println("Symbol {} already defined in current scope", name); 
        exit(-1);
    }
    switch (sym_type)
    {
        case SymbolType::VARIABLE:
        {
            symbols.emplace(name, Symbol{sym_type, type_info, vr});
            break;
        }
        case SymbolType::FUNCTION:
        {
            symbols.emplace(name, Symbol{sym_type, type_info, vr});
            add_function(name);
            break;
        }
    }
}

Symbol SymbolTable::find_symbol(const std::string& name, SymbolType sym_type) 
{

    if (symbols.count(name))
    {
        Symbol sym = symbols.at(name);
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
    function_table[function_name] = function_id;
    function_id++;
    return function_id - 1;
}

uint32_t SymbolTable::get_function_id(const std::string& name)
{
    if (function_table.contains(name))
    {
        return function_table.at(name);
    }
    // TODO ERROR
    std::println("Trying to get function: '{}' id but it's not found", name);
    exit(-1);
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

}

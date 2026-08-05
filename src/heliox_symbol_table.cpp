#include "heliox_symbol_table.hpp"

namespace hx {

uint32_t SYMBOL_ID = 0;

Symbol Symbol::Function(const std::string name, Type return_type, std::vector<Type> param_types, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::FUNCTION, .name = name, .type = return_type, .param_types = param_types, .flags = flags, .id = SYMBOL_ID++};
}

Symbol Symbol::Variable(const std::string name, Type type, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::VARIABLE, .name = name, .type = type, .flags = flags, .id = SYMBOL_ID++};
}

Symbol Symbol::Typedef(const std::string name, Type type, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::VARIABLE, .name = name, .type = type, .flags = flags, .id = SYMBOL_ID++};
}
Symbol Symbol::StructField(const std::string name, Type type, uint32_t alignment, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::STRUCT_FIELD, .name = name, .type = type, .alignment = alignment, .flags = flags, .id = SYMBOL_ID++};
}

sptr<Scope> Scope::get_child()
{
    sptr<Scope> table = std::make_shared<Scope>();
    table->parent = shared_from_this();
    child_scopes.push_back(table);
    return table;
};


bool Scope::symbol_exists_in_current_scope(const std::string& name)
{
   return symbols.find_if([&name](const Symbol& s){return s.name == name; }).has_value();
}

Symbol* Scope::insert_symbol(Symbol symbol)
{
    if (symbol_exists_in_current_scope(symbol.name)) return nullptr;
    return &symbols.push_back(symbol);
}

void Scope::use_scope(sptr<Scope> scope)
{
    using_scopes.push_back(scope);
}

std::optional<Symbol*> Scope::find_function_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::FUNCTION>(name);
}
std::optional<Symbol*> Scope::find_variable_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::VARIABLE>(name);
}
std::optional<Symbol*> Scope::find_typedef_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::TYPEDEF>(name);
}

std::optional<Symbol*> Scope::find_struct_field_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::STRUCT_FIELD>(name);
}

} // namespace hx

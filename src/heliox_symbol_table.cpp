#include "heliox_symbol_table.hpp"

namespace hx {

uint32_t SYMBOL_ID = 0;

Symbol Symbol::Function(const std::string name, Type return_type, std::vector<Type> param_types, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::FUNCTION, .name = name, .type = return_type, .param_types = param_types, .flags = flags, .id = SYMBOL_ID++};
}

Symbol Symbol::Variable(const std::string name, Type type, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::VARIABLE, .name = name, .type = type, .param_types={}, .flags = flags, .id = SYMBOL_ID++};
}

Symbol Symbol::Typedef(const std::string name, Type type, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::TYPEDEF, .name = name, .type = type, .param_types={},  .flags = flags, .id = SYMBOL_ID++};
}
Symbol Symbol::StructField(const std::string name, Type type, uint8_t flags)
{
    return Symbol{.kind = SymbolKind::STRUCT_FIELD, .name = name, .type = type, .param_types={}, .flags = flags, .id = SYMBOL_ID++};
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
    return symbols.contains(name);
}

bool Scope::resolve_type(Type& type) 
{
    // check if type is already resolved
    if (!std::get_if<UnresolvedType>(&type.base)) return true;
    
    auto& unresolved = std::get<UnresolvedType>(type.base);
    Symbol* symbol = find_typedef_symbol(unresolved);
    if (!symbol)
    {
        return false;
    }
    type.base = symbol->type.base;
    // we do += since we could have a type defined as a ptr
    type.ptr_depth += symbol->type.ptr_depth;
    return true;
}

ExpectedSymbol Scope::insert_function_symbol(const std::string& name, Type return_type, std::vector<Type> param_types, uint8_t flags)
{
    if (symbol_exists_in_current_scope(name)) return std::unexpected("Symbol exists in current scope");
    if (!resolve_type(return_type)) {
        return std::unexpected("Return type cannot be resolved");
    }
    for (auto& pt : param_types) 
    {
        if (!resolve_type(pt)) {
            return std::unexpected("Parameter type cannot be resolved");
        }
    }
    auto [it, _] = symbols.insert({name, Symbol::Function(name, return_type, param_types, flags)});
    Symbol* sym = &it->second;
    return sym;
}

ExpectedSymbol Scope::insert_variable_symbol(const std::string& name, Type type) 
{
    if (symbol_exists_in_current_scope(name)) return std::unexpected("Symbol exists in current scope");
    if (!resolve_type(type)) {
        return std::unexpected("Variable type cannot be resolved");
    }
    auto [it, _] = symbols.insert({name, Symbol::Variable(name, type)});
    Symbol* sym = &it->second;
    return sym;
}

ExpectedSymbol Scope::insert_typedef_symbol(const std::string& name, Type type, uint8_t flags) 
{
    if (symbol_exists_in_current_scope(name)) return std::unexpected("Symbol exists in current scope");
    auto [it, _] = symbols.insert({name, Symbol::Typedef(name, type, flags)});
    Symbol* sym = &it->second;
    return sym;
}

void Scope::use_scope(sptr<Scope> scope)
{
    using_scopes.push_back(scope);
}

Symbol* Scope::find_function_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::FUNCTION>(name);
}
Symbol* Scope::find_variable_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::VARIABLE>(name);
}
Symbol* Scope::find_typedef_symbol(const std::string& name)
{
    return find_symbol<SymbolKind::TYPEDEF>(name);
}


Symbol* Scope::find_function_symbol(uptr<identifier_literal_expr>& identifier_literal)
{
    Symbol* sym = find_function_symbol(identifier_literal->name);
    if (!sym) {
        Logger::error(*identifier_literal, std::format("Function '{}' not found", identifier_literal->name));
    }
    return sym;
}

Symbol* Scope::find_variable_symbol(uptr<identifier_literal_expr>& identifier_literal)
{
    Symbol* sym = find_variable_symbol(identifier_literal->name);
    if (!sym) {
        Logger::error(*identifier_literal, std::format("Variable '{}' not found", identifier_literal->name));
    }
    return sym;
}

Symbol* Scope::find_typedef_symbol(uptr<identifier_literal_expr>& identifier_literal)
{
    Symbol* sym = find_typedef_symbol(identifier_literal->name);
    if (!sym) {
        Logger::error(*identifier_literal, std::format("Type '{}' not found", identifier_literal->name));
    }
    return sym;
}

} // namespace hx

#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include "typedefs.hpp"
#include "heliox_types.hpp"

namespace hx
{

inline constexpr uint8_t SF_EXTERN =  1 << 1;
inline constexpr uint8_t SF_VARARGS = 1 << 2;

enum class SymbolKind
{
    TYPEDEF,
    VARIABLE,
    FUNCTION,
    STRUCT_FIELD,
};

struct Symbol
{
    SymbolKind kind;
    std::string name;
    Type type;
    
    std::vector<Type> param_types;
    
    uint32_t alignment;

    uint8_t flags{0};

    uint32_t id;
    
    static Symbol Function(const std::string name, Type return_type, std::vector<Type> param_types, uint8_t flags={});
    static Symbol Variable(const std::string name, Type type, uint8_t flags={});
    static Symbol StructField(const std::string name, Type type, uint32_t alignment, uint8_t flags={});
    static Symbol Typedef(const std::string name, Type type, uint8_t flags={});
};

struct Scope : std::enable_shared_from_this<Scope>
{
    std::string name;
    sptr<Scope> parent;
    BlockVector<Symbol, 64> symbols;
    std::vector<sptr<Scope>> child_scopes;
    std::vector<sptr<Scope>> using_scopes;
    
    sptr<Scope> get_child();
    
    // returns whether symbol was succesfully inserted 
    Symbol* insert_symbol(Symbol symbol);
    bool symbol_exists_in_current_scope(const std::string& name);
    
    void use_scope(sptr<Scope> scope);
    

    // look for symbol of kind "kind"
    template <SymbolKind kind>
    requires (kind == SymbolKind::TYPEDEF 
           || kind == SymbolKind::VARIABLE 
           || kind == SymbolKind::FUNCTION
           || kind == SymbolKind::STRUCT_FIELD)
    std::optional<Symbol*> find_symbol(const std::string& name)
    {
        auto opt = symbols.find_if([&name](const Symbol& s) {return s.name == name;});
        if (opt.has_value()) return &opt.value();
        
        for (const auto& scope : using_scopes)
        {
            auto s = scope->find_symbol<kind>(name);
            if (s.has_value()) return s.value();
        }

        if (parent)
            return parent->find_symbol<kind>(name);

        return std::nullopt;
    }

    std::optional<Symbol*> find_function_symbol(const std::string& name);
    std::optional<Symbol*> find_variable_symbol(const std::string& name);
    std::optional<Symbol*> find_typedef_symbol(const std::string& name);
    std::optional<Symbol*> find_struct_field_symbol(const std::string& name);

};

inline sptr<Scope> create_program_scope()
{
   sptr<Scope> program_scope = std::make_shared<Scope>(); 
   program_scope->name = "program"; 
   for (auto& [str, pt] : primitive_type_map)
   {
       Symbol s;
       s.kind = SymbolKind::TYPEDEF;
       s.name = str;
       s.type = Type{pt, 0};
       program_scope->insert_symbol(s);
   }

  return program_scope;
}


} // namespace hx

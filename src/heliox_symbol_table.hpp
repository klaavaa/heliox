#pragma once

#include "heliox_types.hpp"
#include "heliox_program.hpp"
#include "heliox_pointer.hpp"
#include "heliox_error.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace hx
{
    struct VariableSymbol
    {
        int64_t virtual_register;
        type_data data_type; 
        std::string_view filename;
        uint32_t line_number;
        uint32_t position;
    };

    struct FunctionSymbol
    {
        type_data return_type; 
        std::vector<type_data> parameter_types;
        bool has_varargs;
        std::vector<std::string> module_path;

        bool imported_function;

        std::string_view filename;
        uint32_t line_number;
        uint32_t position;
    };


    struct SymbolTable
    {
        SymbolTable() = default;
        std::unordered_map<std::string, VariableSymbol> variable_symbols; 
        std::unordered_map<std::string, FunctionSymbol> function_symbols; 

        sptr<SymbolTable> parent_table = nullptr;
        std::unordered_map<std::string, sptr<SymbolTable>> child_tables;
        std::unordered_map<std::string, sptr<SymbolTable>> submodule_tables;
    };

    void insert_variable_symbol(sptr<SymbolTable> table, const std::string& name, int64_t virtual_register, const type_data& data_type, std::string_view filename, uint32_t line_number, uint32_t position);
    void insert_function_symbol(sptr<SymbolTable> table, const std::string& name, const type_data& return_type, const std::vector<type_data>& parameter_types, bool has_varargs, const std::vector<std::string>& module_path, bool imported_function, std::string_view filename, uint32_t line_number, uint32_t position);
    sptr<SymbolTable> find_submodule_table(sptr<SymbolTable> table, const std::vector<std::string>& module_path);
    sptr<SymbolTable> get_or_create_submodule_table(sptr<SymbolTable> table, const std::string& module_name);
    void insert_module(sptr<SymbolTable> table, sptr<Module> module, bool is_import);
    FunctionSymbol& find_function_symbol(sptr<SymbolTable> table, const std::string& name, const std::vector<std::string>& module_path, bool find_in_parent_modules);
    sptr<SymbolTable> add_child_table(sptr<SymbolTable> parent, const std::string& name);
    sptr<SymbolTable> get_compound_table(sptr<SymbolTable> current_table);
    void close_compound_table(sptr<SymbolTable> compound_table);
    sptr<SymbolTable> create_global_table_for_translation_unit(const uptr<TranslationUnit>& tu, const uptr<Program>& program);

    VariableSymbol& find_variable_symbol(sptr<SymbolTable> table, const uptr<identifier_literal_expr>& identifier);

    bool table_has_variable_with_vr(sptr<SymbolTable> table, int64_t vr);

    std::vector<std::string> table_get_all_imported_functions(sptr<SymbolTable> table, std::string module_path);

    inline void print_table(sptr<SymbolTable> table)
    {
        for (auto& [n, f] : table->function_symbols)
        {
            std::println("{}()", n);
        }
        for (auto& [n, t] : table->submodule_tables)
        {
            std::println("submodule_table: {}", n);
            print_table(t);
        }
        for (auto& [n, t] : table->child_tables)
        {
            std::println("child_table: {}", n);
            print_table(t);
        }
    }

} // namespace hx
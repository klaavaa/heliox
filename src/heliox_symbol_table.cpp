#include "heliox_symbol_table.hpp"

namespace hx {


void insert_variable_symbol(sptr<SymbolTable> table, const std::string& name, int64_t virtual_register, const type_data& data_type, std::string_view filename, uint32_t line_number, uint32_t position)
{
    if (table->variable_symbols.contains(name))
    {
        Logger::error(filename, line_number, position, HX_SYMBOL_REDEFINITION, "Variable with this name already exists in current scope");
    }

    VariableSymbol symbol{virtual_register, data_type, filename, line_number, position};
    table->variable_symbols.insert({name, symbol});
}

void insert_function_symbol(sptr<SymbolTable> table, const std::string &name, const type_data &return_type, const std::vector<type_data> &parameter_types, bool has_varargs, const std::vector<std::string> &module_path, std::string_view filename, uint32_t line_number, uint32_t position)
{
    if (table->function_symbols.contains(name))
    {
        Logger::error(filename, line_number, position, HX_SYMBOL_REDEFINITION, "Function with this name already exists");
    }
    FunctionSymbol symbol{return_type, parameter_types, has_varargs, module_path, filename, line_number, position};
    table->function_symbols.insert({name, symbol});
}

sptr<SymbolTable> find_submodule_table(sptr<SymbolTable> table, const std::vector<std::string> &module_path)
{
    sptr<SymbolTable> current_table = table;
    for (const auto &module_name : module_path)
    {
        if (!current_table->submodule_tables.contains(module_name))
        {
            Logger::error("", HX_MODULE_NOT_FOUND, "Module not found");
        }
        current_table = current_table->submodule_tables.at(module_name);
    }
    return current_table;
}

sptr<SymbolTable> get_or_create_submodule_table(sptr<SymbolTable> table, const std::string &module_name)
{
    if (module_name.empty())
        return table;
    if (table->submodule_tables.contains(module_name))
    {
        return table->submodule_tables.at(module_name);
    }
    sptr<SymbolTable> submodule_table = std::make_shared<SymbolTable>();
    submodule_table->parent_table = table;
    table->submodule_tables.insert({module_name, submodule_table});
    return submodule_table;
}

void insert_module(sptr<SymbolTable> table, sptr<Module> module)
{
    sptr<SymbolTable> submodule_table = get_or_create_submodule_table(table, module->name);
    for (auto& func : module->functions)
    {
        insert_function_symbol(submodule_table, func->identifier->name, func->type, func->get_parameter_type_data(),
            func->has_varargs, module->get_module_path(), func->filename, func->line_number, func->position);
    }
    for (const auto& [name, submodule] : module->submodules)
    {
        insert_module(submodule_table, submodule);
    }
}

FunctionSymbol& find_function_symbol(sptr<SymbolTable> table, const std::string& name, const std::vector<std::string>& module_path, bool find_in_parent_modules)
{
    sptr<SymbolTable> submodule_table = find_submodule_table(table, module_path);
    do
    { 
    if (submodule_table->function_symbols.contains(name))
    {
        return submodule_table->function_symbols.at(name);
    }
    } while(find_in_parent_modules && (submodule_table = submodule_table->parent_table));

    Logger::error("", HX_SYMBOL_NOT_FOUND, "Function not found");
}

VariableSymbol& find_variable_symbol(sptr<SymbolTable> table, const uptr<identifier_literal_expr>& identifier)
{
    const std::string& name = identifier->name;
    if (table->variable_symbols.contains(name)) 
    {
        return table->variable_symbols.at(name);
    }
    else if (table->parent_table)
    {
        return find_variable_symbol(table->parent_table, identifier);
    }
    Logger::error(*identifier, HX_SYMBOL_NOT_FOUND, "Variable not found");

}

sptr<SymbolTable> add_child_table(sptr<SymbolTable> parent, const std::string& name)
{
    sptr<SymbolTable> child = std::make_shared<SymbolTable>();
    child->parent_table = parent;
    parent->child_tables.insert({name, child});
    return child;
}

sptr<SymbolTable> get_compound_table(sptr<SymbolTable> current_table)
{
    sptr<SymbolTable> compound_table = std::make_shared<SymbolTable>();
    compound_table->parent_table = current_table;
    return compound_table;
}


sptr<SymbolTable> create_global_table_for_translation_unit(const uptr<TranslationUnit>& tu, const uptr<Program>& program)
{
    sptr<SymbolTable> global_table = std::make_shared<SymbolTable>();
    insert_module(global_table, tu->global_module);
    for (const auto& import : tu->imports)
    {
        std::vector<std::string> module_path = import->module_path->module_path;
        module_path.push_back(import->module_path->name);

        sptr<Module> module = program->global_module->find_submodule(module_path);
        insert_module(global_table, module);
    }
    return global_table;
}
} // namespace hx
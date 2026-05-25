#pragma once


namespace hx
{
    struct VariableSymbol
    {
        type_data data_type; 
        int64_t stack_position;

        std::string_view filename;
        uint32_t line_number;
        uint32_t position;
    };

    struct FunctionSymbol
    {
        type_data data_type; 
        std::vector<type_data> parameter_types;
        bool has_varargs;
        std::vector<std::string> module_path;
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
        int64_t current_stack_offset = 0;
    };

    int64_t align_up(int64_t offset, int64_t align)
    {
        return (offset + align - 1) & ~(align - 1);
    }
    
    void insert_variable_symbol(sptr<SymbolTable> table, const std::string& name, const type_data& data_type, std::string_view filename, uint32_t line_number, uint32_t position)
    {
        if (table->variable_symbols.contains(name))
        {
            Logger::error(filename, line_number, position, HX_SYMBOL_REDEFINITION, "Variable with this name already exists in current scope");
        }
        VariableSymbol symbol{data_type, table->current_stack_offset, filename, line_number, position};
        table->current_stack_offset = align_up(table->current_stack_offset + data_type.byte_size, data_type.byte_size);
        table->variable_symbols.insert({name, symbol});
    }

    void insert_function_symbol(sptr<SymbolTable> table, const std::string& name, const type_data& return_type, const std::vector<type_data>& parameter_types, bool has_varargs, const std::vector<std::string>& module_path, std::string_view filename, uint32_t line_number, uint32_t position)
    {
        if (table->function_symbols.contains(name))
        {
            Logger::error(filename, line_number, position, HX_SYMBOL_REDEFINITION, "Function with this name already exists");
        }
        FunctionSymbol symbol{return_type, parameter_types, has_varargs, module_path, filename, line_number, position};
        table->function_symbols.insert({name, symbol});
    }

    sptr<SymbolTable> find_submodule_table(sptr<SymbolTable> table, const std::vector<std::string>& module_path)
    {
        sptr<SymbolTable> current_table = table;
        for (const auto& module_name : module_path)
        {
            if (!current_table->submodule_tables.contains(module_name))
            {
                Logger::error("", HX_MODULE_NOT_FOUND, "Module not found");
            }
            current_table = current_table->submodule_tables.at(module_name);
        }
        return current_table;
    }

    sptr<SymbolTable> get_or_create_submodule_table(sptr<SymbolTable> table, const std::string& module_name)
    {
        if (module_name.empty()) return table;
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

    FunctionSymbol find_function_symbol(sptr<SymbolTable> table, const std::string& name, const std::vector<std::string>& module_path, bool find_in_parent_modules)
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
        compound_table->current_stack_offset = current_table->current_stack_offset;
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
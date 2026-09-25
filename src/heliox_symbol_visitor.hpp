#pragma once

#include "heliox_visitor.hpp"
#include "heliox_error.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_program.hpp"

namespace hx
{

class SymbolVisitor : public Visitor
{
public:
    SymbolVisitor(Program& program)
        : program(program)
    {
        current_scope = program.program_scope;
    }
    
    void populate_toplevel_symbols()
    {

        for (auto& tu : program.translation_units)
        {
            current_scope = tu.global_scope;
            for (auto& statement : tu.statements)
            {
                std::visit(overloads{
                [&tu](uptr<function_statement>& func) 
                {
                    uint8_t flags{};
                    if (func->is_extern) flags |= SF_EXTERN;
                    if (func->has_varargs) flags |= SF_VARARGS;
                    // we'll fill the params in the next pass

                    func->symbol = tu.global_scope->insert_function_symbol(
                            func->name, func->return_type, {}, flags);

                    if (!func->symbol)
                    {
                        Logger::error(*func, std::format("Redefinition of symbol {}", func->name));
                    }
                },
                [&tu](uptr<struct_statement>& struct_s)
                {
                    std::map<std::string, Type> fields;
                    for (const auto& field: struct_s->fields) {
                        if (fields.contains(field->var_name))
                            Logger::error(*struct_s, std::format("Redefinition of field '{}' in struct", field->var_name));
                        fields.insert({field->var_name, field->var_type});
                    }

                    auto* sym = tu.global_scope->insert_typedef_symbol(struct_s->name, Type::Struct(fields), 0);
                    if (!sym)
                        Logger::error(*struct_s, std::format("Redefinition of symbol '{}'", struct_s->name));
                },
                [](auto&&) {}
                }, statement);
            }
        }
    }
    
    void populate_rest_of_symbols_and_resolve_types()
    {
        for (auto& tu : program.translation_units)
        {
            current_scope = tu.global_scope;
            visit_translation_unit(tu);
        }
    }

private:

    void resolve_type(Type& type)
    {
        // check if type is already resolved
        if (!std::get_if<UnresolvedType>(&type.base)) return;
        
        auto& unresolved = std::get<UnresolvedType>(type.base);
        auto symbol_opt = current_scope->find_typedef_symbol(unresolved);
        if (!symbol_opt.has_value())
        {
            Logger::error(filename, line_number, column, std::format("Type '{}' not defined", unresolved));
        }
        Symbol* symbol = symbol_opt.value();
        type.base = symbol->type.base;
        // we do += since we could have a type defined as a ptr
        type.ptr_depth += symbol->type.ptr_depth;
    }
    
    void visit_function_call(uptr<function_call_expr>& fcall) override
    {
        auto symopt = current_scope->find_function_symbol(fcall->name);
        if (!symopt.has_value())
        {
            Logger::error(filename, line_number, column, std::format("Function '{}' not defined", fcall->name));
        }
        fcall->symbol = symopt.value();

        for (auto& param : fcall->parameters)
        {
            visit_expression(param);
        }
    }

    
    void visit_expression_s(uptr<expression_statement>& stat) override
    {
        visit_expression(stat->expr);
    }

    void visit_compound(uptr<compound_statement>& compound) override 
    {
        current_scope = current_scope->get_child(); 
        
        for (auto& statement : compound->statements)
        {
            visit_statement(statement);
        }

        current_scope = current_scope->parent;
    } 
    void visit_explicit_conversion(uptr<explicit_conversion_expr>& explicit_conversion) override
    {
        resolve_type(explicit_conversion->type);
        visit_expression(explicit_conversion->expr);
    }

    void visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) override 
    {
        resolve_type(variable_declaration->var_type);
        variable_declaration->symbol = current_scope->insert_variable_symbol(variable_declaration->var_name,
                variable_declaration->var_type);

        if (!variable_declaration->symbol)
        {
            Logger::error(*variable_declaration, std::format("Redefinition of symbol",
                        variable_declaration->var_name));
        }
    }
    void visit_variable_definition(uptr<variable_definition_statement>& variable_definition) override 
    {
        visit_expression(variable_definition->definition);
        visit_variable_declaration(variable_definition->declaration);
    }
    void visit_conditional(uptr<conditional_statement>& conditional) override
    {
        visit_expression(conditional->condition);
        visit_statement(conditional->then_stat); 
        visit_statement(conditional->else_stat); 
    }
    void visit_while(uptr<while_statement>& while_s) override 
    {
        visit_expression(while_s->condition);
        visit_statement(while_s->loop);
    }
    void visit_for(uptr<for_statement>& for_s) override 
    {
        current_scope = current_scope->get_child();
        visit_statement(for_s->init);
        visit_expression(for_s->condition);
        visit_expression(for_s->iteration);
        visit_statement(for_s->loop);
        current_scope = current_scope->parent;

    }

    void visit_return(uptr<return_statement>& ret_s) override
    {
        visit_expression(ret_s->return_expression);
        ret_s->symbol = current_function_symbol;
    }

    void visit_function(uptr<function_statement>& func) override 
    {

        bool is_toplevel = current_scope->parent == program.program_scope;

        if (!is_toplevel)
        {
            resolve_type(func->return_type);
            uint8_t flags{};
            if (func->is_extern) flags |= SF_EXTERN;
            if (func->has_varargs) flags |= SF_VARARGS;
            func->symbol = current_scope->insert_function_symbol(func->name, func->return_type, {}, flags);
            if (!func->symbol)
            {
                Logger::error(*func, std::format("Redefinition of symbol {}", func->name));
            }
        }
        resolve_type(func->symbol->type);

        current_scope = current_scope->get_child();
        

        for (auto& param : func->params)
        {
            resolve_type(param->var_type);
            param->symbol = current_scope->insert_variable_symbol(param->var_name, param->var_type);
            if (!param->symbol)
            {
                Logger::error(*func, std::format("Redefinition of symbol {}", param->var_name));
            }

            // populate the params of the func symbol
            func->symbol->param_types.push_back(param->symbol->type);

        }
        
        Symbol* previous_function_symbol = current_function_symbol;
        current_function_symbol = func->symbol;

        for (auto& s : func->statements)
        {
            visit_statement(s);
        }

        current_function_symbol = previous_function_symbol;

        current_scope = current_scope->parent;
    }

    void visit_struct(uptr<struct_statement>& struct_s) override
    {
        auto symopt = current_scope->find_typedef_symbol(struct_s->name);
        if (symopt.has_value()) {
            auto* sym = symopt.value();
            resolve_type(sym->type);
        }
        else
        {
            std::map<std::string, Type> fields;
            for (const auto& field: struct_s->fields) {
                if (fields.contains(field->var_name))
                    Logger::error(*struct_s, std::format("Redefinition of field '{}' in struct", field->var_name));
                fields.insert({field->var_name, field->var_type});
            }

            auto* sym = current_scope->insert_typedef_symbol(struct_s->name, Type::Struct(fields), 0);
            if (!sym)
                Logger::error(*struct_s, std::format("Redefinition of symbol '{}'", struct_s->name));
        }
    }
    
    void visit_binop(uptr<binop_expr>& binop) override 
    {
        visit_expression(binop->left);
        
        if (binop->op_token == TokenType::DOT)
        {
            if (!std::holds_alternative<StructType>(effective_type->base))
            {
                Logger::error(*binop, "Type not accessable");
            }
            if (std::holds_alternative<uptr<binop_expr>>(binop->right))
            {
                visit_expression(binop->right);
            } 
            else if (std::holds_alternative<uptr<identifier_literal_expr>>(binop->right))  
            {
                StructType& st = std::get<StructType>(effective_type->base);
                auto& identifier_literal = std::get<uptr<identifier_literal_expr>>(binop->right);
                if (!st.fields.contains(identifier_literal->name)) 
                {
                    auto* ast_node = as_ast_node(binop->right);
                    Logger::error(*ast_node, "Type not accessable");
                }
                resolve_type(st.fields.at(identifier_literal->name));
            }
            else {
                auto* ast_node = as_ast_node(binop->right);
                Logger::error(*ast_node, "Type not accessable");
            }
            return;
        }
        
        visit_expression(binop->right);
    }
    void visit_unary(uptr<unary_expr>& unary) override 
    {
        visit_expression(unary->expr);
    }

    void visit_identifier_literal(uptr<identifier_literal_expr>& i) override
    {
        auto sym_opt = current_scope->find_variable_symbol(i->name);
        if (!sym_opt.has_value())
        {
            Logger::error(filename, line_number, column, std::format("Couldn't find variable '{}'", i->name));
        }
        i->symbol = sym_opt.value();
        effective_type = &i->symbol->type;
    }

    void visit_macro_expr(uptr<macro_expr>& i) override 
    {
        visit_expression(i->argument);
    }

private:
    Program& program;
    sptr<Scope> current_scope;
    Symbol* current_function_symbol;
    Type* effective_type;
};


} // namespace hx

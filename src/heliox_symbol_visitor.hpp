#pragma once

#include "heliox_symbol_table.hpp"
#include "heliox_visitor.hpp"


namespace hx
{
    
    class SymbolVisitor : public Visitor
    {
    public:
        SymbolVisitor(sptr<SymbolTable> global_table) 
            : global_table(global_table)
        {
            
        }
    private:
        void visit_function(uptr<function>& func) override
        {
            std::string func_name = func->identifier->name;
            if (func->is_extern) 
            {
                global_table->add_function_symbol(func_name, 
                        func->type, func->get_parameter_type_data(), func->has_varargs);
                return;
            }
            if (!func->in_module.empty())
            {
                global_table->add_module(func->in_module);
                func_name = func->in_module + "." + func_name;
            }
            global_table->add_function_symbol(func_name, func->type, func->get_parameter_type_data(), func->has_varargs);
        }
        
        
        void visit_int_literal(uptr<int_literal_expr>& int_literal) override{}
        void visit_float_literal(uptr<float_literal_expr>& float_literal) override{}
        void visit_string_literal(uptr<string_literal_expr>& string_literal) override{}
        void visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) override{}
        void visit_binop(uptr<binop_expr>& binop) override{}
        void visit_unary(uptr<unary_expr>& unary) override{}
        void visit_function_call(uptr<function_call_expr>& function_call) override{}
        void visit_explicit_conversion(uptr<explicit_conversion_expr>& explicit_conversion) override{}
        void visit_compound(uptr<compound_statement>& compound) override{}
        void visit_return(uptr<return_statement>& return_s) override{}
        void visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) override{}
        void visit_variable_definition(uptr<variable_definition_statement>& variable_definition) override{}
        void visit_conditional(uptr<conditional_statement>& conditional) override{}
        void visit_while(uptr<while_statement>& while_s) override{}
        void visit_expression_s(uptr<expression_statement>& expr) override{}
        void visit_noop(uptr<noop_statement>& noop) override{}
        void visit_module(uptr<module_statement>& module_s) override{}
        void visit_import(uptr<import_statement>& import_s) override{}

    
        sptr<SymbolTable> global_table;
    };


}


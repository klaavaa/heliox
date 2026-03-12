#pragma once
#include <print>
#include "heliox_token.hpp"
#include "heliox_visitor.hpp"
namespace hx
{

    class debug_visitor : public Visitor
    {
public:
       void visit_function(uptr<function>& func) override 
       {
           std::println("visit::function({})", func->identifier->name);
           std::println("visit::function::params::");
           for (auto& param : func->params)
           {
                visit_variable_declaration(param); 
           }
           std::println("visit::function::stat::");
           for (auto& stat : func->statements)
           {
                visit_statement(stat);
           }
       }

       void visit_int_literal(uptr<int_literal_expr>& int_literal) override 
       {
            std::println("visit::int_literal({})", int_literal->value); 
       }
       void visit_string_literal(uptr<string_literal_expr>& string_literal) override 
       {
            std::println("visit::string_literal({})", string_literal->value); 
       }
       void visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) override
       {
            std::println("visit::identifier_literal({})", identifier_literal->name); 
       }
       void visit_binop(uptr<binop_expr>& binop) override 
       { 
            std::println("visit::binop({})", get_string_from_token_type(binop->op_token));
            std::println("visit::binop::left::");
            visit_expression(binop->left);
            std::println("visit::binop::right::");
            visit_expression(binop->right);
       }
       void visit_unary(uptr<unary_expr>& unary) override 
       {
            std::println("visit::unary({})", get_string_from_token_type(unary->op_token)); 
            std::println("visit::unary::expr::");
            visit_expression(unary->expr);
       }
       void visit_function_call(uptr<function_call_expr>& function_call) override
       {
           std::println("visit::function_call({})", function_call->identifier->name); 
           std::println("visit::function_call::params::");
           for (auto& param : function_call->parameters)
           {
                visit_expression(param);
           }


       }

       void visit_compound(uptr<compound_statement>& compound) override
       {
            std::println("visit::compound()");
            for (auto& stat : compound->statements)
            {
                visit_statement(stat);
            }
       }
       void visit_return(uptr<return_statement>& return_s) override
       {
            std::println("visit::return()");
            visit_expression(return_s->return_expression);
       }
       void visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) override
       {
            std::println("visit::variable_declaration({}, {})", 
                    variable_declaration->var_type.byte_size,
                    variable_declaration->var_identifier->name);
       }
       void visit_variable_definition(uptr<variable_definition_statement>& variable_definition) override
       {
            std::println("visit::variable_definition()");
            visit_variable_declaration(variable_definition->declaration);
            visit_expression(variable_definition->definition);
       }
       void visit_conditional(uptr<conditional_statement>& conditional) override
       {
           std::println("visit::conditional()");
           visit_expression(conditional->condition);
           visit_statement(conditional->then_stat);
           visit_statement(conditional->else_stat);
 
       }
       void visit_while(uptr<while_statement>& while_s) override
       {
            std::println("visit::while()");
            visit_expression(while_s->condition);
            visit_statement(while_s->loop);
       }
       void visit_expression_s(uptr<expression_statement>& expr) override
       {
            std::println("visit::expression_statement()");
            visit_expression(expr->expr);
       }
       void visit_noop(uptr<noop_statement>& noop) override
       {
            std::println("visit::noop()"); 
        }

    };








}

#pragma once
#include "heliox_program.hpp"

namespace hx
{

    class visitor
    {
    public:
        visitor();
        virtual ~visitor();

        virtual void visit_program(uptr<program>& prog);
        virtual void visit_expression(expression& expr);
        virtual void visit_statement(statement& stat);
        
        virtual void visit_function(uptr<function>& func) = 0;

        virtual void visit_int_literal(uptr<int_literal_expr>& int_literal) = 0;
        virtual void visit_string_literal(uptr<string_literal_expr>& string_literal) = 0;
        virtual void visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) = 0; 
        virtual void visit_binop(uptr<binop_expr>& binop) = 0;
        virtual void visit_unary(uptr<unary_expr>& unary) = 0;
        virtual void visit_function_call(uptr<function_call_expr>& function_call) = 0; 

        virtual void visit_compound(uptr<compound_statement>& compound) = 0; 
        virtual void visit_return(uptr<return_statement>& return_s) = 0;
        virtual void visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) = 0;
        virtual void visit_variable_definition(uptr<variable_definition_statement>& variable_definition) = 0;
        virtual void visit_conditional(uptr<conditional_statement>& conditional) = 0;
        virtual void visit_while(uptr<while_statement>& while_s) = 0;
        virtual void visit_expression_s(uptr<expression_statement>& expr) = 0;
        virtual void visit_noop(uptr<noop_statement>& noop) = 0;
  };

}

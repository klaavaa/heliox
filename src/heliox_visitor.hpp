#pragma once
#include "heliox_program.hpp"

namespace hx
{

    class Visitor
    {
    public:
        Visitor();
        virtual ~Visitor();

        virtual void visit_program(Program& prog);
        virtual void visit_translation_unit(TranslationUnit& unit);

        virtual void visit_expression(expression& expr);
        virtual void visit_statement(statement& stat);
        
        virtual void visit_int_literal( [[maybe_unused]] uptr<int_literal_expr>& int_literal) {}
        virtual void visit_float_literal( [[maybe_unused]] uptr<float_literal_expr>& float_literal) {}
        virtual void visit_string_literal( [[maybe_unused]] uptr<string_literal_expr>& string_literal) {}
        virtual void visit_identifier_literal( [[maybe_unused]] uptr<identifier_literal_expr>& identifier_literal) {} 
        virtual void visit_binop( [[maybe_unused]] uptr<binop_expr>& binop) {}
        virtual void visit_unary( [[maybe_unused]] uptr<unary_expr>& unary) {}
        virtual void visit_function_call( [[maybe_unused]] uptr<function_call_expr>& function_call) {} 
        virtual void visit_explicit_conversion( [[maybe_unused]] uptr<explicit_conversion_expr>& explicit_conversion) {}
        virtual void visit_noop_e( [[maybe_unused]] uptr<noop_expression>& noop) {}

        virtual void visit_compound( [[maybe_unused]] uptr<compound_statement>& compound) {} 
        virtual void visit_return( [[maybe_unused]] uptr<return_statement>& return_s) {}
        virtual void visit_variable_declaration( [[maybe_unused]] uptr<variable_declaration_statement>& variable_declaration) {}
        virtual void visit_variable_definition( [[maybe_unused]] uptr<variable_definition_statement>& variable_definition) {}
        virtual void visit_conditional( [[maybe_unused]] uptr<conditional_statement>& conditional) {}
        virtual void visit_while( [[maybe_unused]] uptr<while_statement>& while_s) {}
        virtual void visit_for( [[maybe_unused]] uptr<for_statement>& for_s) {}
        virtual void visit_expression_s( [[maybe_unused]] uptr<expression_statement>& expr) {}
        virtual void visit_noop_s( [[maybe_unused]] uptr<noop_statement>& noop) {}
        virtual void visit_break( [[maybe_unused]] uptr<break_statement>& break_s) {}
        virtual void visit_continue( [[maybe_unused]] uptr<continue_statement>& continue_s) {}
        virtual void visit_asm( [[maybe_unused]] uptr<asm_statement>& asm_s) {}
        virtual void visit_function( [[maybe_unused]] uptr<function_statement>& func) {}
    protected:
        const ast_node* as_ast_node(expression& expr) const;
        const ast_node* as_ast_node(statement& stat) const;
    protected:
        std::string_view filename;
        uint32_t line_number;
        uint32_t column;
  };

}

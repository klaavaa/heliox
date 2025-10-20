#pragma once
#include "heliox_visitor.hpp"
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include <queue>

namespace hx
{

class instruction_generator : public visitor
{
    public:


        void emit_instruction(instruction_triplet triplet, uint32_t increment = 1);
       
        void visit_program(uptr<program>& prog) override;
        void visit_function(uptr<function>& func) override;
        
        void visit_int_literal(uptr<int_literal_expr>& int_literal) override;
        
        void visit_string_literal(uptr<string_literal_expr>& string_literal) override;
        void visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) override;
        void visit_binop(uptr<binop_expr>& binop) override;
        void visit_unary(uptr<unary_expr>& unary) override;
        void visit_function_call(uptr<function_call_expr>& function_call) override;
        void visit_compound(uptr<compound_statement>& compound) override;
        void visit_return(uptr<return_statement>& return_s) override;
        void visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) override;
        
        void visit_variable_definition(uptr<variable_definition_statement>& variable_definition) override;
        void visit_conditional(uptr<conditional_statement>& conditional) override;
        void visit_while(uptr<while_statement>& while_s) override;
        void visit_expression_s(uptr<expression_statement>& expr) override;
        void visit_noop(uptr<noop_statement>& noop) override;
        

        std::queue<instruction_function> instruction_functions;
        virtual_register current_virtual_register = 0;
        virtual_register effective_register = 0;
        
        uptr<symbol_table> global_table;
        symbol_table* current_table;
};

}

#pragma once
#include "heliox_visitor.hpp"
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include <queue>

namespace hx
{

class InstructionGenerator : public Visitor
{
    public:


        void emit_instruction(InstructionTriplet triplet, uint32_t increment = 1);
       
        void visit_program(uptr<Program>& prog) override;
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
        
        void calculate_live_ranges();

        InstructionData instruction_data;
        virtual_register current_virtual_register = 0;
        virtual_register effective_register = 0;
        uint32_t instruction_count = 0; 
        uptr<SymbolTable> global_table;
        SymbolTable* current_table;

};

}

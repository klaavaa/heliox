#pragma once
#include "heliox_visitor.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_instructions.hpp"

namespace hx
{

class InstructionGenerator : public Visitor
{
public:
    InstructionGenerator(uptr<TranslationUnit> _translation_unit, sptr<SymbolTable> _global_table);    

    IRUnit generate_instructions();



private:
    void emit_instruction(const IRInstruction& instruction, virtual_register inc = 1, bool set_effective = true);

    void register_vr_type(virtual_register vr, const type_data& type);
    const type_data& get_vr_type(virtual_register vr) const;

    std::string get_module_prefix() const;

    void visit_function(uptr<function>& func) override;
    void visit_int_literal(uptr<int_literal_expr>& int_literal) override;
    void visit_float_literal(uptr<float_literal_expr>& float_literal) override{}
    void visit_string_literal(uptr<string_literal_expr>& string_literal) override;
    void visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal)  override{}
    void visit_binop(uptr<binop_expr>& binop) override{}
    void visit_unary(uptr<unary_expr>& unary) override{}
    void visit_function_call(uptr<function_call_expr>& function_call) override;
    void visit_explicit_conversion(uptr<explicit_conversion_expr>& explicit_conversion) override{}
    void visit_compound(uptr<compound_statement>& compound) override;
    void visit_return(uptr<return_statement>& return_s) override;
    void visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) override{}
    void visit_variable_definition(uptr<variable_definition_statement>& variable_definition) override{}
    void visit_conditional(uptr<conditional_statement>& conditional) override{}
    void visit_while(uptr<while_statement>& while_s) override{}
    void visit_expression_s(uptr<expression_statement>& expr) override;
    void visit_noop(uptr<noop_statement>& noop) override{}
    void visit_import(uptr<import_statement>& import_s) override{}
private:
    uptr<TranslationUnit> translation_unit;
    sptr<SymbolTable> global_table;
    sptr<SymbolTable> current_table;

    IRUnit ir_unit;
    std::vector<IRInstruction> current_instructions;

    virtual_register current_register = 1;
    virtual_register effective_register = 1;
};




} // namespace hx
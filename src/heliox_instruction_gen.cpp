#include "heliox_instruction_gen.hpp"

namespace hx
{
InstructionGenerator::InstructionGenerator(uptr<TranslationUnit> _translation_unit, sptr<SymbolTable> _global_table)
    : translation_unit(std::move(_translation_unit)), global_table(std::move(_global_table)), current_table(global_table)
{

}

IRUnit InstructionGenerator::generate_instructions()
{
    visit_module(translation_unit->global_module);
    return ir_unit;
}

std::string InstructionGenerator::get_module_prefix() const
{
    std::string prefix;
    for (const auto& module_name : current_module_path)
    {
        prefix += module_name + ".";
    }
    return prefix;
}

void InstructionGenerator::emit_instruction(const IRInstruction& instruction, virtual_register inc, bool set_effective)
{
    current_instructions.push_back(instruction);

    if (set_effective)
    {
        effective_register = current_register;
    }

    current_register += inc;
}
void InstructionGenerator::register_vr_type(virtual_register vr, const type_data& type)
{
    ir_unit.virtual_register_types.insert({vr, type});
}

const type_data& InstructionGenerator::get_vr_type(virtual_register vr) const
{
    return ir_unit.virtual_register_types.at(vr); 
}


void InstructionGenerator::visit_function(uptr<function>& func)
{
    IRFunction ir_func;
    ir_func.name = get_module_prefix() + func->identifier->name;

    for (auto& statement : func->statements)
    {
        visit_statement(statement);
    }

    ir_func.instructions = std::move(current_instructions); 
    ir_unit.ir_functions.push_back(std::move(ir_func));
}


void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call)
{
    FunctionSymbol& func_symbol = find_function_symbol(global_table, function_call->identifier->name, function_call->in_module, function_call->find_in_parent_modules); 

    const size_t param_count = function_call->parameters.size();
    if ((param_count != func_symbol.parameter_types.size() && !func_symbol.has_varargs) || param_count < func_symbol.parameter_types.size())
    {
        Logger::error(*function_call, HX_INVALID_ARGUMENTS, "Function call argument count does not match function signature");
    }

    // parameter instructions
    for (size_t i = 0; i < param_count; i++)
    {
        
        auto& param = function_call->parameters[i];
        visit_expression(param);
        IRInstruction push_arg_instruction{IRInstructionType::PUSH_ARG, 0, effective_register, i};
        emit_instruction(push_arg_instruction, 0, false);
    }


    // call instruction
    virtual_register name_id = ir_unit.allocate_function_name(function_call->identifier->get_full_name());
    IRInstruction call_instruction{IRInstructionType::FUNCTION_CALL, current_register, name_id, 0};
    register_vr_type(current_register, func_symbol.return_type);
    emit_instruction(call_instruction);

}

void InstructionGenerator::visit_compound(uptr<compound_statement>& compound)
{
    current_table = get_compound_table(current_table);
    for (auto& statement : compound->statements)
    {
        visit_statement(statement);
    }
    current_table = current_table->parent_table;
}

void InstructionGenerator::visit_expression_s(uptr<expression_statement>& expr)
{
    visit_expression(expr->expr);
}

void InstructionGenerator::visit_string_literal(uptr<string_literal_expr>& string_literal) 
{
    virtual_register literal_location = ir_unit.allocate_string_literal(string_literal->value);
    IRInstruction load_string{IRInstructionType::LOAD_STRING, current_register, literal_location, 0};
    register_vr_type(current_register, type_data{primitive_type::U8, 1});
    emit_instruction(load_string);
}
void InstructionGenerator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    virtual_register int_value = std::stoull(int_literal->value);
    IRInstruction load_int{IRInstructionType::LOAD_INT, current_register, int_value, 0};
    register_vr_type(current_register, type_data{primitive_type::I64, 0});
    emit_instruction(load_int);
}


void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    // todo check current function return type
    visit_expression(return_s->return_expression);
    IRInstruction return_inst{IRInstructionType::RETURN, current_register, effective_register, 0};
    register_vr_type(current_register, get_vr_type(effective_register));
    emit_instruction(return_inst);
}

} // namespace hx
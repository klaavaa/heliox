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

void InstructionGenerator::emit_instruction(const IRInstruction& instruction, int64_t inc, bool set_effective)
{
    current_function.instructions.push_back(instruction);
    if (set_effective)
    {
        effective_register = current_register;
    }

    current_register.value += inc;
}
void InstructionGenerator::register_vr_type(IROperand vr, const type_data& type)
{
    if (vr.value < 0)
    {
        Logger::error("", -1, std::format("Trying to register type for virtual_register: {}", vr.value));
    }
    current_function.virtual_register_types.insert({vr.value, type});
}
void InstructionGenerator::register_vr_type(IROperand vr, IROperand from_vr)
{
    register_vr_type(vr, get_vr_type(from_vr));
}

const type_data& InstructionGenerator::get_vr_type(IROperand vr) const
{
    if (!current_function.virtual_register_types.contains(vr.value))
    {
        Logger::error("", -1, std::format("Virtual register: {} doesn't have a registered type", vr));
    }

    return current_function.virtual_register_types.at(vr.value); 
}


void InstructionGenerator::visit_function(uptr<function>& func)
{
    current_function = IRFunction{};
    current_function.name = get_module_prefix() + func->identifier->name;
    current_function.is_extern = func->is_extern;

    current_table = add_child_table(global_table, current_function.name);
    
    current_register.value = 0;
    effective_register.value = 0;

    for (auto& statement : func->statements)
    {
        visit_statement(statement);
    }
    
    current_table = global_table;

    ir_unit.ir_functions.push_back(std::move(current_function));
}


void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call)
{
    FunctionSymbol& func_symbol = find_function_symbol(global_table, function_call->identifier->name, function_call->in_module, function_call->find_in_parent_modules); 

    const size_t param_count = function_call->parameters.size();
    if ((param_count != func_symbol.parameter_types.size() && !func_symbol.has_varargs) || param_count < func_symbol.parameter_types.size())
    {
        Logger::error(*function_call, HX_INVALID_ARGUMENTS, "Function call argument count does not match function signature");
    }

    std::vector<IROperand> arg_vregs;
    // parameter instructions
    for (size_t i = 0; i < param_count; i++)
    {
        auto& param = function_call->parameters[i];
        visit_expression(param);
        arg_vregs.push_back(effective_register);
    }
    for (size_t i = 0; i < arg_vregs.size(); i++)
    {
        IROperand arg_vreg = arg_vregs[i];
        IRInstruction push_arg_instruction(IRInstructionType::MOV_ARG, current_register, arg_vreg, {IROperandKind::ARG_NUMBER, (int64_t)i});
        register_vr_type(current_register, arg_vreg);
        emit_instruction(push_arg_instruction);
    }

    // call instruction

    std::string function_full_name;
    for (const auto& s : function_call->in_module)
    {
        function_full_name += s + ".";
    }
    function_full_name += function_call->identifier->name;
    int64_t name_id = (int64_t)ir_unit.allocate_function_name(function_full_name);
    IRInstruction call_instruction(IRInstructionType::FUNCTION_CALL, current_register, IROperand::Literal(name_id), IROperand::None());
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
    int64_t literal_location = (int64_t)ir_unit.allocate_string_literal(string_literal->value);
    IRInstruction load_string(IRInstructionType::LOAD_MEM_INDEX, current_register, IROperand::Literal(literal_location), IROperand::None());
    register_vr_type(current_register, type_data{primitive_type::U8, 1});
    emit_instruction(load_string);
}
void InstructionGenerator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    int64_t int_value = std::stoll(int_literal->value);
    IRInstruction load_int(IRInstructionType::LOAD_IMMEDIATE, current_register, IROperand::Immediate(int_value), IROperand::None());
    register_vr_type(current_register, type_data{primitive_type::I64, 0});
    emit_instruction(load_int);
}

void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal)
{
    const VariableSymbol& var_sym = find_variable_symbol(current_table, identifier_literal);
    
    IRInstruction mov(IRInstructionType::MOV, current_register, IROperand::Vr(var_sym.virtual_register), IROperand::None());
    register_vr_type(current_register, mov.src1);
    emit_instruction(mov);
}

void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    // todo check current function return type
    visit_expression(return_s->return_expression);
    IRInstruction return_inst(IRInstructionType::RETURN, current_register, effective_register, IROperand::None());
    register_vr_type(current_register, effective_register);
    emit_instruction(return_inst);
}

void InstructionGenerator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration)
{
    insert_variable_symbol(current_table, variable_declaration->var_identifier->name, current_register.value,
        variable_declaration->var_type, variable_declaration->filename, variable_declaration->line, variable_declaration->position);
    
    register_vr_type(current_register, variable_declaration->var_type);

    effective_register = current_register;
    current_register.value++;
}

void InstructionGenerator::visit_variable_definition(uptr<variable_definition_statement>& variable_definition)
{
    visit_expression(variable_definition->definition);
    IROperand expression_vr = effective_register;
    visit_variable_declaration(variable_definition->declaration);
    const VariableSymbol& var_symbol = find_variable_symbol(current_table, variable_definition->declaration->var_identifier);
    IRInstruction store(IRInstructionType::MOV, IROperand::Vr(var_symbol.virtual_register), expression_vr, IROperand::None());
    emit_instruction(store, 0, false);
}

int64_t InstructionGenerator::unwrap_assigment(TokenType op_token, IROperand left_register, IROperand right_register)
{

    // maybe do this shit in the parser, problem is that they are all unique ptrs Q_Q
    Logger::not_implemented();

}
void InstructionGenerator::emit_assignment(TokenType op_token, expression& left_side, expression& right_side)
{
    visit_expression(right_side);
    IROperand right_register = effective_register;


    std::visit(
        overloads{
        [this, op_token, &right_register](uptr<identifier_literal_expr>& identifier)
        {
            const VariableSymbol& var_sym = find_variable_symbol(current_table, identifier);
            IRInstruction write_var(IRInstructionType::MOV, IROperand::Vr(var_sym.virtual_register), right_register, IROperand::None());
            emit_instruction(write_var, 0);
        },
        [this, op_token, &right_register](uptr<unary_expr>& unary) 
        {
            if (unary->op_token != TokenType::MULTIPLY)
            {
                Logger::error(*unary, HX_ILLEGAL_ASSIGNMENT, "Tried to assign a non-assignable value");
            }

            visit_expression(unary->expr);
            IRInstruction write_mem(IRInstructionType::STORE_MEM, effective_register, right_register, IROperand::None());
            emit_instruction(write_mem, 0, false);
            
        },

        [](auto& expr) { Logger::error(*expr, HX_ILLEGAL_ASSIGNMENT, "Tried to assign a non-assignable value"); }
        }, left_side
    );
}

IRInstructionType InstructionGenerator::get_ir_binop_instruction(TokenType op_token, IROperand left_register)
{
    const auto& data_type = get_vr_type(left_register);
    IRInstructionType ir_instruction_type;
    // FLOAT OPERATIONS
    if (!is_integer_type(data_type))
    {
        switch (op_token)
        {
        default:
            goto unknown_binop_operator;
        }
        return ir_instruction_type;
    }

    // INTEGER OPERATIONS
    switch (op_token)
    {
    case TokenType::PLUS:
        ir_instruction_type = IRInstructionType::IADD;
        break;
    case TokenType::MINUS:
        ir_instruction_type = IRInstructionType::ISUB;
        break;
    case TokenType::MULTIPLY:
        ir_instruction_type = IRInstructionType::IMUL;
        break;
    case TokenType::DIVIDE:
        ir_instruction_type = IRInstructionType::IDIV;
        break;
    default:
        goto unknown_binop_operator;
    }

    return ir_instruction_type;
    
unknown_binop_operator:
    Logger::error("", HX_UNKNOWN_OPERATOR, "Not a valid binop operator");
}

void InstructionGenerator::visit_binop(uptr<binop_expr>& binop)
{
    if (is_equals_operator(binop->op_token))
    {
        // todo += -= etc (maybe unwrap in parser?)
        emit_assignment(binop->op_token, binop->left, binop->right);
        return;
    }

    visit_expression(binop->left);
    IROperand left_register = effective_register;
    visit_expression(binop->right);
    IROperand right_register = effective_register;
    // todo implicit conversion, actual check
    if (is_integer_type(get_vr_type(left_register)) != is_integer_type(get_vr_type(right_register)))
    {
        Logger::error(*binop, HX_ILLEGAL_BINARY_OPERATION, "binary operation types dont match");
    }

    IRInstructionType instruction_type = get_ir_binop_instruction(binop->op_token, left_register);

    IRInstruction binop_inst(instruction_type, current_register, left_register, right_register);
    register_vr_type(current_register, left_register);
    emit_instruction(binop_inst);

}

void InstructionGenerator::visit_unary(uptr<unary_expr>& unary) 
{
    visit_expression(unary->expr);

    switch (unary->op_token)
    {
    case TokenType::MULTIPLY:
    {
        IRInstruction deref(IRInstructionType::DEREF, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, get_vr_type(effective_register).deref());
        emit_instruction(deref);
        break;
    }
    default:
        Logger::error(*unary, HX_UNKNOWN_OPERATOR, "Unknown unary operator");
    }

}


} // namespace hx
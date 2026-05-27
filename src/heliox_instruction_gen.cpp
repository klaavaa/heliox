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
    current_instructions.push_back(instruction);

    if (set_effective)
    {
        effective_register = current_register;
    }

    current_register += inc;
}
void InstructionGenerator::register_vr_type(int64_t vr, const type_data& type)
{
    if (vr < 0)
    {
        Logger::error("", -1, std::format("Trying to register type for virtual_register: {}", vr));
    }
    ir_unit.virtual_register_types.insert({vr, type});
}
void InstructionGenerator::register_vr_type(int64_t vr, int64_t from_vr)
{
    register_vr_type(vr, get_vr_type(from_vr));
}

const type_data& InstructionGenerator::get_vr_type(int64_t vr) const
{
    if (!ir_unit.virtual_register_types.contains(vr))
    {
        Logger::error("", -1, std::format("Virtual register: {} doesn't have a registered type", vr));
    }

    return ir_unit.virtual_register_types.at(vr); 
}


void InstructionGenerator::visit_function(uptr<function>& func)
{
    IRFunction ir_func;
    ir_func.name = get_module_prefix() + func->identifier->name;

    current_table = add_child_table(global_table, ir_func.name);
   
    for (auto& statement : func->statements)
    {
        visit_statement(statement);
    }
    
    current_table = global_table;

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

    std::vector<int64_t> arg_vregs;
    // parameter instructions
    for (size_t i = 0; i < param_count; i++)
    {
        auto& param = function_call->parameters[i];
        visit_expression(param);
        arg_vregs.push_back(effective_register);
    }
    for (size_t i = 0; i < arg_vregs.size(); i++)
    {
        int64_t arg_vreg = arg_vregs[i];
        IRInstruction push_arg_instruction(IRInstructionType::MOV_ARG, current_register, arg_vreg, (int64_t)i);
        register_vr_type(current_register, arg_vreg);
        emit_instruction(push_arg_instruction);
    }

    // call instruction
    int64_t name_id = (int64_t)ir_unit.allocate_function_name(function_call->identifier->get_full_name());
    IRInstruction call_instruction(IRInstructionType::FUNCTION_CALL, current_register, name_id, -1);
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
    IRInstruction load_string(IRInstructionType::LOAD_MEM_INDEX, current_register, literal_location, -1);
    register_vr_type(current_register, type_data{primitive_type::U8, 1});
    emit_instruction(load_string);
}
void InstructionGenerator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    int64_t int_value = std::stoll(int_literal->value);
    IRInstruction load_int(IRInstructionType::LOAD_IMMEDIATE, current_register, int_value, -1);
    register_vr_type(current_register, type_data{primitive_type::I64, 0});
    emit_instruction(load_int);
}

void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal)
{
    const VariableSymbol& var_sym = find_variable_symbol(current_table, identifier_literal);
    
    IRInstruction mov(IRInstructionType::MOV, current_register, var_sym.virtual_register, 0);
    register_vr_type(current_register, var_sym.virtual_register);
    emit_instruction(mov);
}

void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    // todo check current function return type
    visit_expression(return_s->return_expression);
    IRInstruction return_inst(IRInstructionType::RETURN, current_register, effective_register, -1);
    register_vr_type(current_register, effective_register);
    emit_instruction(return_inst);
}

void InstructionGenerator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration)
{
    insert_variable_symbol(current_table, variable_declaration->var_identifier->name, current_register,
        variable_declaration->var_type, variable_declaration->filename, variable_declaration->line, variable_declaration->position);
    
    register_vr_type(current_register, variable_declaration->var_type);

    effective_register = current_register;
    current_register++;
}

void InstructionGenerator::visit_variable_definition(uptr<variable_definition_statement>& variable_definition)
{
    visit_expression(variable_definition->definition);
    int64_t expression_vr = effective_register;
    visit_variable_declaration(variable_definition->declaration);
    const VariableSymbol& var_symbol = find_variable_symbol(current_table, variable_definition->declaration->var_identifier);
    IRInstruction store(IRInstructionType::MOV, var_symbol.virtual_register, expression_vr, -1);
    emit_instruction(store, 0, false);
}

void InstructionGenerator::emit_assignment(expression& left_side, expression& right_side)
{
    visit_expression(right_side);
    int64_t right_register = effective_register;

    std::visit(
        overloads{
        [this, right_register](uptr<identifier_literal_expr>& identifier)
        {
            const VariableSymbol& var_sym = find_variable_symbol(current_table, identifier);
            
            IRInstruction write_var(IRInstructionType::MOV, var_sym.virtual_register, right_register, -1);
            emit_instruction(write_var, 0);
        },
        [this, right_register](uptr<unary_expr>& unary) 
        {
            if (unary->op_token != TokenType::MULTIPLY)
            {
                Logger::error(*unary, HX_ILLEGAL_ASSIGNMENT, "Tried to assign a non-assignable value");
            }

            visit_expression(unary->expr);
            IRInstruction write_mem(IRInstructionType::STORE_MEM, effective_register, right_register, -1);
            emit_instruction(write_mem, 0, false);
            
        },

        [](auto& expr) { Logger::error(*expr, HX_ILLEGAL_ASSIGNMENT, "Tried to assign a non-assignable value"); }
        }, left_side
    );
}

IRInstructionType InstructionGenerator::get_ir_binop_instruction(TokenType op_token, int64_t left_register)
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
        emit_assignment(binop->left, binop->right);
        return;
    }

    visit_expression(binop->left);
    int64_t left_register = effective_register;
    visit_expression(binop->right);
    int64_t right_register = effective_register;
    // todo implicit conversion, actual check
    if (is_integer_type(get_vr_type(left_register)) != is_integer_type(get_vr_type(right_register)))
    {
        Logger::error(*binop, HX_ILLEGAL_BINARY_OPERATION, "binary operation types dont match");
    }

    IRInstructionType instruction_type = get_ir_binop_instruction(binop->op_token, left_register);

    IRInstruction binop_inst(instruction_type, left_register, right_register, -1);
    emit_instruction(binop_inst, 0);
    effective_register = left_register;

}

void InstructionGenerator::visit_unary(uptr<unary_expr>& unary) 
{
    visit_expression(unary->expr);

    switch (unary->op_token)
    {
    case TokenType::MULTIPLY:
    {
        IRInstruction deref(IRInstructionType::DEREF, current_register, effective_register, -1);
        register_vr_type(current_register, get_vr_type(effective_register).deref());
        emit_instruction(deref);
        break;
    }
    default:
        Logger::error(*unary, HX_UNKNOWN_OPERATOR, "Unknown unary operator");
    }

}


} // namespace hx
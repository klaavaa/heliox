#include "heliox_instruction_gen.hpp"
#include "heliox_operator.hpp"
#include "heliox_registerdata.hpp"

namespace hx
{
InstructionGenerator::InstructionGenerator(TranslationUnit& _translation_unit)
    : translation_unit(_translation_unit)
{

}

IRUnit InstructionGenerator::generate_instructions()
{
    visit_translation_unit(translation_unit);
    return ir_unit;
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
void InstructionGenerator::register_vr_type(IROperand vr, const Type& type)
{
    if (vr.value < 0)
    {
        Logger::internal_error();
    }
    current_function.virtual_register_types.insert({vr.value, type});
}
void InstructionGenerator::register_vr_type(IROperand vr, IROperand from_vr)
{
    register_vr_type(vr, get_vr_type(from_vr));
}

bool InstructionGenerator::has_vr_type(IROperand vr)
{
    return current_function.virtual_register_types.contains(vr.value);
}

const Type& InstructionGenerator::get_vr_type(IROperand vr) const
{
    std::println("vrval {} ", vr.value);
    if (!current_function.virtual_register_types.contains(vr.value))
    {
        Logger::internal_error();
    }

    return current_function.virtual_register_types.at(vr.value); 
}


void InstructionGenerator::visit_function(uptr<function_statement>& func)
{
    current_register.value = 0;
    effective_register.value = 0;
    current_function = IRFunction{};
    current_function.name = func->symbol.name;
    current_function.is_extern = func->is_extern;

    if (current_function.is_extern)
    {
        ir_unit.ir_functions.push_back(std::move(current_function));
        return;
    }
    
    for (size_t i = 0; i < func->params.size(); i++)
    {
        const auto& param = func->params[i];
        auto src = IROperand::Vr(current_register.value);
        current_register.value++;
        auto dst = IROperand::Vr(current_register.value);
        register_vr_type(src, param->symbol.type);
        register_vr_type(dst, param->symbol.type);
        symbol_id_to_vr.emplace(param->symbol.id, dst.value);
        current_function.vrs_with_variables.insert(dst.value);
        emit_instruction(IRInstruction(IRInstructionType::REGISTER_ARG, dst, src, IROperand::Arg((int64_t)i)));
    }

    for (auto& statement : func->statements)
    {
        visit_statement(statement);
    }

    ir_unit.ir_functions.push_back(std::move(current_function));
}


void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call)
{
    //FunctionSymbol& func_symbol = find_function_symbol(global_table, function_call->identifier->name, function_call->in_module, function_call->find_in_parent_modules); 
    Symbol& func_symbol = function_call->symbol;
    const size_t param_count = function_call->parameters.size();
    if ((param_count != func_symbol.param_types.size() && !(func_symbol.flags | SF_VARARGS) ) || param_count < func_symbol.param_types.size())
    {
        Logger::error(*function_call, "Function call argument count does not match function signature");
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
        IRInstructionType mov_type;
        effective_register = arg_vreg;
        if (i < func_symbol.param_types.size())
        {
             mov_type = IRInstructionType::MOV_ARG;
             emit_implicit_conversion(*function_call, arg_vreg, func_symbol.param_types[i]);
        }
        else
        {
            mov_type = IRInstructionType::MOV_VARARG;
            Type vararg_type = get_vr_type(arg_vreg);
            if (is_float_type(vararg_type) && vararg_type.byte_size() == 4)
            {
                emit_implicit_conversion(*function_call, arg_vreg, TYPE_F64);
            }
        }
        IRInstruction push_arg_instruction(mov_type, current_register, effective_register, {IROperandKind::ARG_NUMBER, (int64_t)i});
        register_vr_type(current_register, effective_register);
        emit_instruction(push_arg_instruction);
    }

    // call instruction
    int64_t name_id = (int64_t)ir_unit.allocate_function_name(function_call->name);
    IRInstruction call_instruction(IRInstructionType::FUNCTION_CALL, current_register, IROperand::Literal(name_id), IROperand::None());
    register_vr_type(current_register, func_symbol.type);
    emit_instruction(call_instruction);

    if (func_symbol.type.byte_size() != 0)
    {
        IRInstruction mov(IRInstructionType::MOV, current_register, effective_register, IROperand::None());;
        register_vr_type(current_register, effective_register);
        emit_instruction(mov);
    }

}

void InstructionGenerator::visit_compound(uptr<compound_statement>& compound)
{
    for (auto& statement : compound->statements)
    {
        visit_statement(statement);
    }
}

void InstructionGenerator::visit_expression_s(uptr<expression_statement>& expr)
{
    visit_expression(expr->expr);
}

void InstructionGenerator::visit_string_literal(uptr<string_literal_expr>& string_literal) 
{
    int64_t literal_location = (int64_t)ir_unit.allocate_string_literal(string_literal->value);
    IRInstruction load_string(IRInstructionType::LOAD_MEM_INDEX, current_register, IROperand::Literal(literal_location), IROperand::None());
    register_vr_type(current_register, Type{PrimitiveType::U8, 1});
    emit_instruction(load_string);
}
void InstructionGenerator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    int64_t int_value = std::stoll(int_literal->value);
    IRInstruction load_int(IRInstructionType::LOAD_IMMEDIATE, current_register, IROperand::Immediate(int_value), IROperand::None());
    register_vr_type(current_register, TYPE_I64);
    emit_instruction(load_int);
}

void InstructionGenerator::visit_float_literal(uptr<float_literal_expr>& float_literal)
{
    int64_t literal_location = (int64_t)ir_unit.allocate_float64_literal(float_literal->value);
    IRInstruction load_float(IRInstructionType::LOAD_FLOAT64, current_register, IROperand::Literal(literal_location), IROperand::None());
    register_vr_type(current_register, TYPE_F64);
    emit_instruction(load_float);
}

void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal)
{
    int64_t vr = symbol_id_to_vr.at(identifier_literal->symbol.id);
    IRInstruction mov(IRInstructionType::MOV, current_register, IROperand::Vr(vr), IROperand::None());
    register_vr_type(current_register, mov.src1);
    emit_instruction(mov);
}

void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    // todo check current function return type
    visit_expression(return_s->return_expression);
    if (return_s->symbol.type.byte_size() != 0)
        emit_implicit_conversion(*return_s, effective_register, return_s->symbol.type);
    IRInstruction return_inst(IRInstructionType::RETURN, current_register, effective_register, IROperand::None());
    register_vr_type(current_register, effective_register);
    
    emit_instruction(return_inst);
}

void InstructionGenerator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration)
{
    symbol_id_to_vr.emplace(variable_declaration->symbol.id, current_register.value);
    register_vr_type(current_register, variable_declaration->var_type);
    current_function.vrs_with_variables.insert(current_register.value);
    effective_register = current_register;
    current_register.value++;
}

void InstructionGenerator::visit_variable_definition(uptr<variable_definition_statement>& variable_definition)
{
    visit_expression(variable_definition->definition);
    IROperand expression_vr = effective_register;
    visit_variable_declaration(variable_definition->declaration);
    
    if (get_vr_type(expression_vr) != get_vr_type(effective_register))
    {
        emit_implicit_conversion(*variable_definition, expression_vr, get_vr_type(effective_register));
        expression_vr = effective_register;
    }

    int64_t vr = symbol_id_to_vr.at(variable_definition->declaration->symbol.id);
    IRInstruction store(IRInstructionType::MOV, IROperand::Vr(vr), expression_vr, IROperand::None());
    emit_instruction(store, 0, false);

}

void InstructionGenerator::emit_implicit_conversion(const ast_node& node, IROperand vr, const Type& type_to)
{
    const Type& type_from = get_vr_type(vr);

    if (!is_implicit_conversion_possible(type_from, type_to))
    {
        //todo cool error text like from i32* to f32 or etc
        Logger::error(node, "Implicit conversion not possible");
    }

    effective_register = vr;
    if (is_integer_type(type_from))
    {
        // todo: not sure if this is the best way to go about this
        // current_function.virtual_register_types.at(vr.value) = type_to;
        return;
    }
    else if (is_float_type(type_from))
    {
        
        if (type_to.byte_size() == 8 && type_from.byte_size() == 4)
        {
            IRInstruction conversion(IRInstructionType::CONVERT_F32_TO_F64, current_register, vr, IROperand::None());
            register_vr_type(current_register, TYPE_F64);
            emit_instruction(conversion);
        }
        else if (type_to.byte_size() == 4 && type_from.byte_size() == 8)
        {
            IRInstruction conversion(IRInstructionType::CONVERT_F64_TO_F32, current_register, vr, IROperand::None());
            register_vr_type(current_register, TYPE_F32);
            emit_instruction(conversion);
        }
        return;
    }

    Logger::not_implemented();

}

void InstructionGenerator::unwrap_assigment(TokenType op_token, expression& left_side, expression& right_side)
{

    TokenType unwrap_token;
    switch (op_token)
    {
    case TokenType::PLUSEQUALS:
    {
        unwrap_token = TokenType::PLUS;
        break;
    }
    case TokenType::MINUSEQUALS:
    {
        unwrap_token = TokenType::MINUS;
        break;
    }
    case TokenType::MULEQUALS:
    {
        unwrap_token = TokenType::MULTIPLY;
        break;
    }
    case TokenType::DIVEQUALS:
    {
        unwrap_token = TokenType::DIVIDE;
        break;
    }
    case TokenType::MODEQUALS:
    {
        unwrap_token = TokenType::MODULO;
        break;
    }

    default:
        return;
    }

    auto binop = std::make_unique<binop_expr>("", 0, 0, std::move(left_side), std::move(right_side), unwrap_token);
    visit_binop(binop);
    left_side = std::move(binop->left);
    right_side = std::move(binop->right);

}
void InstructionGenerator::emit_assignment(TokenType op_token, expression& left_side, expression& right_side)
{
    visit_expression(right_side);

    unwrap_assigment(op_token, left_side, right_side);
    IROperand right_register = effective_register;

    std::visit(
        overloads{
        [this, op_token, &right_register](uptr<identifier_literal_expr>& identifier)
        {
            emit_implicit_conversion(*identifier, right_register, identifier->symbol.type);
            int64_t vr = symbol_id_to_vr.at(identifier->symbol.id);
            IRInstruction write_var(IRInstructionType::MOV, IROperand::Vr(vr), effective_register, IROperand::None());
            emit_instruction(write_var, 0);
        },
        [this, op_token, &right_register](uptr<unary_expr>& unary) 
        {
            if (unary->op_token != TokenType::MULTIPLY)
            {
                Logger::error(*unary, "Tried to assign a non-assignable value");
            }

            visit_expression(unary->expr);
            IROperand left_side = effective_register;
            
            auto try_deref = get_vr_type(left_side).deref();
            if (!try_deref.has_value()) Logger::error(*unary, "Cannot dereference non-pointer type");
            Type deref_type = try_deref.value();
            emit_implicit_conversion(*unary, right_register, deref_type);
            IRInstruction write_mem(IRInstructionType::STORE_MEM, left_side, effective_register, IROperand::None());
            emit_instruction(write_mem, 0, false);
            
        },

        [](auto& expr) { Logger::error(*expr, "Tried to assign a non-assignable value"); }
        }, left_side
    );
}

IRInstructionType InstructionGenerator::get_ir_binop_instruction(TokenType op_token, IROperand left_register)
{
    const auto& data_type = get_vr_type(left_register);
    IRInstructionType ir_instruction_type;
    // FLOAT OPERATIONS
    if (is_float_type(data_type))
    {
        if (data_type.byte_size() == 4)
        {
            switch (op_token)
            {
            case TokenType::PLUS:
                ir_instruction_type = IRInstructionType::F32ADD;
                break;
            case TokenType::MINUS:
                ir_instruction_type = IRInstructionType::F32SUB;
                break;
            case TokenType::MULTIPLY:
                ir_instruction_type = IRInstructionType::F32MUL;
                break;
            case TokenType::DIVIDE:
                ir_instruction_type = IRInstructionType::F32DIV;
                break;
            case TokenType::DOUBLE_EQU:
                ir_instruction_type = IRInstructionType::F32CMP_EQU;
                register_vr_type(current_register, TYPE_I8);
                break;
            case TokenType::NEQU:
                ir_instruction_type = IRInstructionType::F32CMP_NEQU;
                register_vr_type(current_register, TYPE_I8);
                break;
            case TokenType::LT:
                ir_instruction_type = IRInstructionType::F32CMP_LT;
                register_vr_type(current_register, TYPE_I8);
                break;
            case TokenType::GT:
                ir_instruction_type = IRInstructionType::F32CMP_GT;
                register_vr_type(current_register, TYPE_I8);
                break;
            case TokenType::LTE:
                ir_instruction_type = IRInstructionType::F32CMP_LTE;
                register_vr_type(current_register, TYPE_I8);
                break;
            case TokenType::GTE:
                ir_instruction_type = IRInstructionType::F32CMP_GTE;
                register_vr_type(current_register, TYPE_I8);
                break;
            default:
                goto unknown_binop_operator;
            }
        }
        else
        {
        switch (op_token)
        {
        case TokenType::PLUS:
            ir_instruction_type = IRInstructionType::F64ADD;
            break;
        case TokenType::MINUS:
            ir_instruction_type = IRInstructionType::F64SUB;
            break;
        case TokenType::MULTIPLY:
            ir_instruction_type = IRInstructionType::F64MUL;
            break;
        case TokenType::DIVIDE:
            ir_instruction_type = IRInstructionType::F64DIV;
            break;

        case TokenType::DOUBLE_EQU:
            ir_instruction_type = IRInstructionType::F64CMP_EQU;
            register_vr_type(current_register, TYPE_I8);
            break;
        case TokenType::NEQU:
            ir_instruction_type = IRInstructionType::F64CMP_NEQU;
            register_vr_type(current_register, TYPE_I8);
            break;
        case TokenType::LT:
            ir_instruction_type = IRInstructionType::F64CMP_LT;
            register_vr_type(current_register, TYPE_I8);
            break;
        case TokenType::GT:
            ir_instruction_type = IRInstructionType::F64CMP_GT;
            register_vr_type(current_register, TYPE_I8);
            break;
        case TokenType::LTE:
            ir_instruction_type = IRInstructionType::F64CMP_LTE;
            register_vr_type(current_register, TYPE_I8);
            break;
        case TokenType::GTE:
            ir_instruction_type = IRInstructionType::F64CMP_GTE;
            register_vr_type(current_register, TYPE_I8);
            break;

        default:
            goto unknown_binop_operator;
        }
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
    case TokenType::MODULO:
        ir_instruction_type = IRInstructionType::IMOD;
        break;
    case TokenType::DOUBLE_EQU:
        ir_instruction_type = IRInstructionType::ICMP_EQU;
        register_vr_type(current_register, TYPE_I8);
        break;
    case TokenType::NEQU:
        ir_instruction_type = IRInstructionType::ICMP_NEQU;
        register_vr_type(current_register, TYPE_I8);
        break;
    case TokenType::LT:
        ir_instruction_type = IRInstructionType::ICMP_LT;
        register_vr_type(current_register, TYPE_I8);
        break;
    case TokenType::GT:
        ir_instruction_type = IRInstructionType::ICMP_GT;
        register_vr_type(current_register, TYPE_I8);
        break;
    case TokenType::LTE:
        ir_instruction_type = IRInstructionType::ICMP_LTE;
        register_vr_type(current_register, TYPE_I8);
        break;
    case TokenType::GTE:
        ir_instruction_type = IRInstructionType::ICMP_GTE;
        register_vr_type(current_register, TYPE_I8);
        break;
    case TokenType::BITWISE_AND:
        ir_instruction_type = IRInstructionType::BITWISE_AND;
        break;
    case TokenType::BITWISE_OR:
        ir_instruction_type = IRInstructionType::BITWISE_OR;
        break;
    case TokenType::BITWISE_XOR:
        ir_instruction_type = IRInstructionType::BITWISE_XOR;
        break;

    case TokenType::SHIFT_LEFT:
        ir_instruction_type = IRInstructionType::SHIFT_LEFT;
        break;
    case TokenType::SHIFT_RIGHT:
        ir_instruction_type = IRInstructionType::SHIFT_RIGHT;
        break;

    default:
        goto unknown_binop_operator;
    }

    return ir_instruction_type;
    
unknown_binop_operator:
    Logger::error(filename, line_number, column, "Not a valid binop operator");
}

void InstructionGenerator::visit_binop(uptr<binop_expr>& binop)
{
    if (is_equals_operator(binop->op_token))
    {
        // todo += -= etc (maybe unwrap in parser?)
        emit_assignment(binop->op_token, binop->left, binop->right);
        return;
    }

    // logical operators need to be handled with their own logic
    if (binop->op_token == TokenType::LOGICAL_AND || binop->op_token == TokenType::LOGICAL_OR)
    {
        visit_logical_binop(binop->op_token, binop->left, binop->right);
        return;
    }

    visit_expression(binop->left);
    IROperand left_register = effective_register;
    visit_expression(binop->right);
    IROperand right_register = effective_register;

    // try implicit conversion
    if (get_vr_type(left_register) != get_vr_type(right_register))
    {
        emit_implicit_conversion(*binop, right_register, get_vr_type(left_register));
    }

    IRInstructionType instruction_type = get_ir_binop_instruction(binop->op_token, left_register);

    IRInstruction binop_inst(instruction_type, current_register, left_register, effective_register);
    if (!has_vr_type(current_register))
    {
        register_vr_type(current_register, left_register);
    }
    
    emit_instruction(binop_inst);

}

void InstructionGenerator::visit_unary(uptr<unary_expr>& unary) 
{
    if (unary->op_token == TokenType::BITWISE_AND)
    {
        if (!std::holds_alternative<uptr<identifier_literal_expr>>(unary->expr))
        {
            Logger::error(*unary, "Trying to get the address of a non-literal");
        }
        auto& identifier_literal = std::get<uptr<identifier_literal_expr>>(unary->expr);
        int64_t vr = symbol_id_to_vr.at(identifier_literal->symbol.id);
        IROperand var_vr = IROperand::Vr(vr);
        IRInstruction addr_of(IRInstructionType::ADDR_OF, current_register, var_vr, IROperand::None());
        register_vr_type(current_register, identifier_literal->symbol.type.get_ptr());
        emit_instruction(addr_of);
        return;
    }

    visit_expression(unary->expr);

    switch (unary->op_token)
    {
    case TokenType::MULTIPLY:
    {
        IRInstruction deref(IRInstructionType::DEREF, current_register, effective_register, IROperand::None());
        auto try_deref = get_vr_type(effective_register).deref();
        if (!try_deref.has_value()) Logger::error(*unary, "Cannot dereference non-pointer type");
        Type deref_type = try_deref.value();
        register_vr_type(current_register, deref_type);
        emit_instruction(deref);
        break;
    }
    case TokenType::BITWISE_NOT:
    {
        IRInstruction bitwise_not(IRInstructionType::BITWISE_NOT, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, effective_register);
        emit_instruction(bitwise_not);
        break;
    }
    case TokenType::NOT:
    {
        auto zero_label = IROperand::Label(next_label++);    
        auto end_label = IROperand::Label(next_label++);    
        IRInstruction jmp_if(IRInstructionType::JMP_IF, IROperand::None(), effective_register, zero_label);
        emit_instruction(jmp_if, 0, false);
        IRInstruction mov_one(IRInstructionType::MOV, current_register, IROperand::Immediate(1), IROperand::None());
        emit_instruction(mov_one);
        IRInstruction jmp(IRInstructionType::JMP, IROperand::None(), IROperand::None(), end_label);
        emit_instruction(jmp, 0, false);
        IRInstruction zero_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), zero_label);
        emit_instruction(zero_label_inst, 0, false);
        IRInstruction mov_zero(IRInstructionType::MOV, effective_register, IROperand::Immediate(0), IROperand::None());
        emit_instruction(mov_zero, 0, false);
        IRInstruction end_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), end_label);
        emit_instruction(end_label_inst, 0, false);
        register_vr_type(effective_register, TYPE_I8); 
        break;
    }
    default:
        Logger::error(*unary, "Unknown unary operator");
    }

}

void InstructionGenerator::visit_conditional(uptr<conditional_statement>& conditional)
{
    visit_expression(conditional->condition);
    auto if_end_label = IROperand::Label(next_label++);
    IRInstruction jmp_not(IRInstructionType::JMP_IF_NOT, IROperand::None(), effective_register, if_end_label);
    emit_instruction(jmp_not, 0, false);
        
    visit_statement(conditional->then_stat);

    IRInstruction if_end(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), if_end_label);

    if (!std::get_if<uptr<noop_statement>>(&conditional->else_stat))
    {
        auto else_end_label = IROperand::Label(next_label++);
        IRInstruction jmp(IRInstructionType::JMP, IROperand::None(), IROperand::None(), else_end_label);
        emit_instruction(jmp, 0, false);
        emit_instruction(if_end, 0, false);

        visit_statement(conditional->else_stat);

        IRInstruction else_end(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), else_end_label);
        emit_instruction(else_end, 0, false);
        return;
    }

    emit_instruction(if_end);

}

void InstructionGenerator::visit_while(uptr<while_statement>& while_s)
{
    auto begin_label = IROperand::Label(next_label++);
    auto end_label = IROperand::Label(next_label++);

    auto previous_continue_label = loop_continue_label;
    auto previous_break_label = loop_break_label;

    loop_continue_label = begin_label;
    loop_break_label = end_label;

    IRInstruction begin_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), begin_label);
    emit_instruction(begin_label_inst, 0, false);

    visit_expression(while_s->condition);

    IRInstruction jmp_not(IRInstructionType::JMP_IF_NOT, IROperand::None(), effective_register, end_label);
    emit_instruction(jmp_not, 0, false);

    visit_statement(while_s->loop);

    
    IRInstruction jmp_begin(IRInstructionType::JMP, IROperand::None(), IROperand::None(), begin_label);
    emit_instruction(jmp_begin, 0, false);

    IRInstruction end_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), end_label);
    emit_instruction(end_label_inst, 0, false);

    loop_continue_label = previous_continue_label;
    loop_break_label = previous_break_label;

}

void InstructionGenerator::visit_for(uptr<for_statement>& for_s)
{

    auto begin_label = IROperand::Label(next_label++);
    auto iteration_label = IROperand::Label(next_label++);
    auto end_label = IROperand::Label(next_label++);
    
    auto previous_continue_label = loop_continue_label;
    auto previous_break_label = loop_break_label;

    loop_continue_label = iteration_label;
    loop_break_label = end_label;

    visit_statement(for_s->init);  

    IRInstruction begin_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), begin_label);
    emit_instruction(begin_label_inst, 0, false);

    visit_expression(for_s->condition);
    IRInstruction jmp_not(IRInstructionType::JMP_IF_NOT, IROperand::None(), effective_register, end_label);
    emit_instruction(jmp_not, 0, false);

    visit_statement(for_s->loop);
    IRInstruction iteration_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), iteration_label);
    emit_instruction(iteration_label_inst, 0, false);

    visit_expression(for_s->iteration);
    IRInstruction jmp_begin(IRInstructionType::JMP, IROperand::None(), IROperand::None(), begin_label);
    emit_instruction(jmp_begin, 0, false);
    emit_instruction(IRInstruction(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), end_label), 0, false);

    loop_continue_label = previous_continue_label;
    loop_break_label = previous_break_label;

}

void InstructionGenerator::visit_break(uptr<break_statement>& break_s) 
{
    if (loop_break_label.kind == IROperandKind::NONE) Logger::error(*break_s, "break statement not inside a loop");
    IRInstruction jmp(IRInstructionType::JMP, IROperand::None(), IROperand::None(), loop_break_label);
    emit_instruction(jmp, 0, false);
}

void InstructionGenerator::visit_continue(uptr<continue_statement>& continue_s)
{
    if (loop_continue_label.kind == IROperandKind::NONE) Logger::error(*continue_s, "continue statement not inside a loop");
    IRInstruction jmp(IRInstructionType::JMP, IROperand::None(), IROperand::None(), loop_continue_label);
    emit_instruction(jmp, 0, false);
}
void InstructionGenerator::visit_asm(uptr<asm_statement>& asm_s)
{
    std::vector<Register> clobbered_registers;
    for (const auto& clobber_str : asm_s->clobbered_registers)
    {
        Register r = get_register_from_string(clobber_str->value);
        clobbered_registers.push_back(r);
    }
    
    std::string parsed_string;
    const auto& actual_string = asm_s->asm_body->value;
    for (size_t i = 0; i < actual_string.size();i++)
    {
        if (actual_string[i] == '\\' && actual_string[i + 1] == 'n')
        {
            parsed_string += '\n';
            i++;
            continue;
        }
        parsed_string += actual_string[i];
    }
    
    AssemblyBlock asm_block(parsed_string, clobbered_registers);
    size_t id = ir_unit.allocate_assembly_block(asm_block);

    IRInstruction asm_inst(IRInstructionType::INLINE_ASM, current_register, IROperand::ASMBlock(id), IROperand::None());
    emit_instruction(asm_inst);
}

void InstructionGenerator::visit_logical_binop(TokenType op_token, expression& left, expression& right)
{
    if (op_token == TokenType::LOGICAL_AND)
    {
        
        auto zero_label = IROperand::Label(next_label++);
        auto end_label = IROperand::Label(next_label++);

        visit_expression(left);
        IRInstruction jmp_if_zero1(IRInstructionType::JMP_IF_NOT, IROperand::None(), effective_register, zero_label);
        emit_instruction(jmp_if_zero1, 0, false);
        visit_expression(right);
        IRInstruction jmp_if_zero2(IRInstructionType::JMP_IF_NOT, IROperand::None(), effective_register, zero_label);
        emit_instruction(jmp_if_zero2, 0, false);

        IRInstruction mov_one(IRInstructionType::MOV, current_register, IROperand::Immediate(1), IROperand::None());
        emit_instruction(mov_one);

        IRInstruction jmp_end(IRInstructionType::JMP, IROperand::None(), IROperand::None(), end_label);
        emit_instruction(jmp_end, 0, false);

        IRInstruction zero_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), zero_label);
        emit_instruction(zero_label_inst, 0, false);
        IRInstruction mov_zero(IRInstructionType::MOV, effective_register, IROperand::Immediate(0), IROperand::None());
        emit_instruction(mov_zero, 0, false);
        IRInstruction end_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), end_label);
        emit_instruction(end_label_inst, 0, false);

        register_vr_type(effective_register, TYPE_I8);
        return;
    }
    if (op_token == TokenType::LOGICAL_OR)
    {
        auto one_label = IROperand::Label(next_label++);
        auto end_label = IROperand::Label(next_label++);

        visit_expression(left);
        IRInstruction jmp_if1(IRInstructionType::JMP_IF, IROperand::None(), effective_register, one_label);
        emit_instruction(jmp_if1, 0, false);
        visit_expression(right);
        IRInstruction jmp_if2(IRInstructionType::JMP_IF, IROperand::None(), effective_register, one_label);
        emit_instruction(jmp_if2, 0, false);

        IRInstruction mov_zero(IRInstructionType::MOV, current_register, IROperand::Immediate(0), IROperand::None());
        emit_instruction(mov_zero);

        IRInstruction jmp_end(IRInstructionType::JMP, IROperand::None(), IROperand::None(), end_label);
        emit_instruction(jmp_end, 0, false);

        IRInstruction one_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), one_label);
        emit_instruction(one_label_inst, 0, false);
        IRInstruction mov_one(IRInstructionType::MOV, effective_register, IROperand::Immediate(1), IROperand::None());
        emit_instruction(mov_one, 0, false);
        IRInstruction end_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), end_label);
        emit_instruction(end_label_inst, 0, false);

        register_vr_type(effective_register, TYPE_I8);
        return;
    }

    Logger::not_implemented();
}

} // namespace hx

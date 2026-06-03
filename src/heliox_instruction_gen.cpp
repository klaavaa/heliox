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
    
    
    auto all_imported_func_names = table_get_all_imported_functions(global_table, "");

    for (const auto& fname : all_imported_func_names)
    {
        IRFunction ir_func;
        ir_func.name = fname;
        ir_func.is_extern = true;
        ir_unit.ir_functions.push_back(ir_func);
    }

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

bool InstructionGenerator::has_vr_type(IROperand vr)
{
    return current_function.virtual_register_types.contains(vr.value);
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
    current_register.value = 0;
    effective_register.value = 0;

    current_function = IRFunction{};
    current_function.name = get_module_prefix() + func->identifier->name;
    current_function.is_extern = func->is_extern;

    current_function_node = func.get();

    if (current_function.is_extern)
    {
        ir_unit.ir_functions.push_back(std::move(current_function));
        return;
    }

    current_table = add_child_table(global_table, current_function.name);
    
    
    for (size_t i = 0; i < func->params.size(); i++)
    {
        const auto& param = func->params[i];
        auto src = IROperand::Vr(current_register.value);
        current_register.value++;
        auto dst = IROperand::Vr(current_register.value);
        register_vr_type(src, param->var_type);
        register_vr_type(dst, param->var_type);
        insert_variable_symbol(current_table, param->var_identifier->name, dst.value, param->var_type, param->filename, param->line, param->position);
        emit_instruction(IRInstruction(IRInstructionType::REGISTER_ARG, dst, src, IROperand::Arg((int64_t)i)));
    }

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
        IRInstructionType mov_type;
        effective_register = arg_vreg;
        if (i < func_symbol.parameter_types.size())
        {
             mov_type = IRInstructionType::MOV_ARG;
             emit_implicit_conversion(*function_call, arg_vreg, func_symbol.parameter_types[i]);
        }
        else
        {
            mov_type = IRInstructionType::MOV_VARARG;
            type_data vararg_type = get_vr_type(arg_vreg);
            if (is_float_type(vararg_type) && vararg_type.byte_size == 4)
            {
                emit_implicit_conversion(*function_call, arg_vreg, type_data{primitive_type::F64, 0});
            }
        }
        IRInstruction push_arg_instruction(mov_type, current_register, effective_register, {IROperandKind::ARG_NUMBER, (int64_t)i});
        register_vr_type(current_register, effective_register);
        emit_instruction(push_arg_instruction);
    }

    // call instruction
    std::string function_full_name;
    for (const auto& s : func_symbol.module_path)
    {
        function_full_name += s + ".";
    }
    function_full_name += function_call->identifier->name;
    int64_t name_id = (int64_t)ir_unit.allocate_function_name(function_full_name);
    IRInstruction call_instruction(IRInstructionType::FUNCTION_CALL, current_register, IROperand::Literal(name_id), IROperand::None());
    register_vr_type(current_register, func_symbol.return_type);
    emit_instruction(call_instruction);

    if (func_symbol.return_type.byte_size != 0)
    {
        IRInstruction mov(IRInstructionType::MOV, current_register, effective_register, IROperand::None());;
        register_vr_type(current_register, effective_register);
        emit_instruction(mov);
    }


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

void InstructionGenerator::visit_float_literal(uptr<float_literal_expr>& float_literal)
{
    int64_t literal_location = (int64_t)ir_unit.allocate_float64_literal(float_literal->value);
    IRInstruction load_float(IRInstructionType::LOAD_FLOAT64, current_register, IROperand::Literal(literal_location), IROperand::None());
    register_vr_type(current_register, type_data{primitive_type::F64, 0});
    emit_instruction(load_float);
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
    emit_implicit_conversion(*return_s, effective_register, current_function_node->type);
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
    
    if (get_vr_type(expression_vr) != get_vr_type(effective_register))
    {
        emit_implicit_conversion(*variable_definition, expression_vr, get_vr_type(effective_register));
        expression_vr = effective_register;
    }

    const VariableSymbol& var_symbol = find_variable_symbol(current_table, variable_definition->declaration->var_identifier);

    IRInstruction store(IRInstructionType::MOV, IROperand::Vr(var_symbol.virtual_register), expression_vr, IROperand::None());
    emit_instruction(store, 0, false);

}

void InstructionGenerator::emit_implicit_conversion(const ast_node& node, IROperand vr, const type_data type_to)
{
    const type_data& type_from = get_vr_type(vr);

    if (!is_implicit_conversion_possible(type_from, type_to))
    {
        //todo cool error text like from i32* to f32 or etc
        Logger::error(node, HX_IMPLICIT_CONVERSION_NOT_POSSIBLE, "Implicit conversion not possible");
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
        
        if (type_to.byte_size == 8 && type_from.byte_size == 4)
        {
            IRInstruction conversion(IRInstructionType::CONVERT_F32_TO_F64, current_register, vr, IROperand::None());
            register_vr_type(current_register, {primitive_type::F64, 0});
            emit_instruction(conversion);
        }
        else if (type_to.byte_size == 4 && type_from.byte_size == 8)
        {
            IRInstruction conversion(IRInstructionType::CONVERT_F64_TO_F32, current_register, vr, IROperand::None());
            register_vr_type(current_register, {primitive_type::F32, 0});
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
            const VariableSymbol& var_sym = find_variable_symbol(current_table, identifier);
            emit_implicit_conversion(*identifier, right_register, var_sym.data_type);

            IRInstruction write_var(IRInstructionType::MOV, IROperand::Vr(var_sym.virtual_register), effective_register, IROperand::None());
            emit_instruction(write_var, 0);
        },
        [this, op_token, &right_register](uptr<unary_expr>& unary) 
        {
            if (unary->op_token != TokenType::MULTIPLY)
            {
                Logger::error(*unary, HX_ILLEGAL_ASSIGNMENT, "Tried to assign a non-assignable value");
            }

            visit_expression(unary->expr);
            IROperand left_side = effective_register;

            emit_implicit_conversion(*unary, right_register, get_vr_type(left_side).deref());
            IRInstruction write_mem(IRInstructionType::STORE_MEM, left_side, effective_register, IROperand::None());
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
    if (is_float_type(data_type))
    {
        if (data_type.byte_size == 4)
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
                register_vr_type(current_register, type_data{primitive_type::I8, 0});
                break;
            case TokenType::NEQU:
                ir_instruction_type = IRInstructionType::F32CMP_NEQU;
                register_vr_type(current_register, type_data{primitive_type::I8, 0});
                break;
            case TokenType::LT:
                ir_instruction_type = IRInstructionType::F32CMP_LT;
                register_vr_type(current_register, type_data{primitive_type::I8, 0});
                break;
            case TokenType::GT:
                ir_instruction_type = IRInstructionType::F32CMP_GT;
                register_vr_type(current_register, type_data{primitive_type::I8, 0});
                break;
            case TokenType::LTE:
                ir_instruction_type = IRInstructionType::F32CMP_LTE;
                register_vr_type(current_register, type_data{primitive_type::I8, 0});
                break;
            case TokenType::GTE:
                ir_instruction_type = IRInstructionType::F32CMP_GTE;
                register_vr_type(current_register, type_data{primitive_type::I8, 0});
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
            register_vr_type(current_register, type_data{primitive_type::I8, 0});
            break;
        case TokenType::NEQU:
            ir_instruction_type = IRInstructionType::F64CMP_NEQU;
            register_vr_type(current_register, type_data{primitive_type::I8, 0});
            break;
        case TokenType::LT:
            ir_instruction_type = IRInstructionType::F64CMP_LT;
            register_vr_type(current_register, type_data{primitive_type::I8, 0});
            break;
        case TokenType::GT:
            ir_instruction_type = IRInstructionType::F64CMP_GT;
            register_vr_type(current_register, type_data{primitive_type::I8, 0});
            break;
        case TokenType::LTE:
            ir_instruction_type = IRInstructionType::F64CMP_LTE;
            register_vr_type(current_register, type_data{primitive_type::I8, 0});
            break;
        case TokenType::GTE:
            ir_instruction_type = IRInstructionType::F64CMP_GTE;
            register_vr_type(current_register, type_data{primitive_type::I8, 0});
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
        register_vr_type(current_register, type_data{primitive_type::I8, 0});
        break;
    case TokenType::NEQU:
        ir_instruction_type = IRInstructionType::ICMP_NEQU;
        register_vr_type(current_register, type_data{primitive_type::I8, 0});
        break;
    case TokenType::LT:
        ir_instruction_type = IRInstructionType::ICMP_LT;
        register_vr_type(current_register, type_data{primitive_type::I8, 0});
        break;
    case TokenType::GT:
        ir_instruction_type = IRInstructionType::ICMP_GT;
        register_vr_type(current_register, type_data{primitive_type::I8, 0});
        break;
    case TokenType::LTE:
        ir_instruction_type = IRInstructionType::ICMP_LTE;
        register_vr_type(current_register, type_data{primitive_type::I8, 0});
        break;
    case TokenType::GTE:
        ir_instruction_type = IRInstructionType::ICMP_GTE;
        register_vr_type(current_register, type_data{primitive_type::I8, 0});
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
            Logger::error(*unary, HX_ILLEGAL_ADDR_OF, "Trying to get the address of a non-literal");
        }
        auto& identifier_literal = std::get<uptr<identifier_literal_expr>>(unary->expr);
        VariableSymbol var_sym = find_variable_symbol(current_table, identifier_literal);
        IROperand var_vr = IROperand::Vr(var_sym.virtual_register);
        IRInstruction addr_of(IRInstructionType::ADDR_OF, current_register, var_vr, IROperand::None());
        register_vr_type(current_register, var_sym.data_type.get_ptr_type());
        emit_instruction(addr_of);
        return;
    }

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
    case TokenType::BITWISE_NOT:
    {
        IRInstruction bitwise_not(IRInstructionType::BITWISE_NOT, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, effective_register);
        emit_instruction(bitwise_not);
        break;
    }
    default:
        Logger::error(*unary, HX_UNKNOWN_OPERATOR, "Unknown unary operator");
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

}

void InstructionGenerator::visit_for(uptr<for_statement>& for_s)
{
    auto begin_label = IROperand::Label(next_label++);
    auto end_label = IROperand::Label(next_label++);
    
    current_table = get_compound_table(current_table);
    visit_statement(for_s->init);  

    IRInstruction begin_label_inst(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), begin_label);
    emit_instruction(begin_label_inst, 0, false);

    visit_expression(for_s->condition);
    IRInstruction jmp_not(IRInstructionType::JMP_IF_NOT, IROperand::None(), effective_register, end_label);
    emit_instruction(jmp_not, 0, false);

    visit_statement(for_s->loop);
    visit_expression(for_s->iteration);
    IRInstruction jmp_begin(IRInstructionType::JMP, IROperand::None(), IROperand::None(), begin_label);
    emit_instruction(jmp_begin, 0, false);
    emit_instruction(IRInstruction(IRInstructionType::LABEL, IROperand::None(), IROperand::None(), end_label), 0, false);
    current_table = current_table->parent_table;
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

        register_vr_type(effective_register, type_data{primitive_type::I8, 0});
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

        register_vr_type(effective_register, type_data{primitive_type::I8, 0});
        return;
    }

    Logger::not_implemented();
}

} // namespace hx

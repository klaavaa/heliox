#include "heliox_instruction_gen.hpp"
#include "heliox_operator.hpp"
#include "heliox_registerdata.hpp"

namespace hx
{
InstructionGenerator::InstructionGenerator(TranslationUnit& _translation_unit)
    :    translation_unit(_translation_unit)
{

}

IRUnit InstructionGenerator::generate_instructions()
{
    current_scope = translation_unit.global_scope;
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
    
    std::vector<Type> param_types{};
    for (auto& p : func->params) 
    {
        param_types.push_back(p->var_type);
    }
    
    int8_t flags{};
    if (func->is_extern)
        flags |= SF_EXTERN;
    if (func->has_varargs)
        flags |= SF_VARARGS;

    ExpectedSymbol expected =
        current_scope->insert_function_symbol(func->name, func->return_type, param_types, flags);

    if (!expected.has_value()) {
        Logger::error(*func, expected.error());
    }
    current_function.symbol = expected.value();
    

    if (current_function.symbol->flags & SF_EXTERN)
    {
        ir_unit.ir_functions.push_back(current_function);
        return;
    }
    
    current_scope = current_scope->get_child();

    
    for (size_t i = 0; i < func->params.size(); i++)
    {
        const auto& param = func->params[i];
        
        
        ExpectedSymbol expected =
            current_scope->insert_variable_symbol(param->var_name, param->var_type);

        if (!expected.has_value()) {
            Logger::error(*func, expected.error());
        }

        Symbol* param_symbol = expected.value();

        auto src = IROperand::Vr(current_register.value);
        current_register.value++;
        auto dst = IROperand::Vr(current_register.value);
        register_vr_type(src, param_symbol->type);
        register_vr_type(dst, param_symbol->type);
        symbol_id_to_vr.emplace(param_symbol->id, dst.value);
        current_function.vrs_with_variables.insert(dst.value);
        emit_instruction(IRInstruction(IRInstructionType::REGISTER_ARG, dst, src, IROperand::Arg((int64_t)i)));
    }

    for (auto& statement : func->statements)
    {
        visit_statement(statement);
    }

    ir_unit.ir_functions.push_back(current_function);

    current_scope = current_scope->parent;
}


void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call)
{
    Symbol* symbol = current_scope->find_function_symbol(function_call->name);
    if (!symbol) {
        Logger::error(*function_call, "Function not found");
    }

    const size_t param_count = function_call->parameters.size();
    if ((param_count != symbol->param_types.size() && !(symbol->flags | SF_VARARGS) ) || param_count < symbol->param_types.size())
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
        if (i < symbol->param_types.size())
        {
             mov_type = IRInstructionType::MOV_ARG;
             emit_implicit_conversion(*function_call, arg_vreg, symbol->param_types[i]);
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
    IRInstruction call_instruction(IRInstructionType::FUNCTION_CALL, current_register, IROperand::LiteralLocation(name_id), IROperand::None());
    register_vr_type(current_register, symbol->type);
    emit_instruction(call_instruction);

    if (symbol->type.byte_size() != 0)
    {
        IRInstruction mov(IRInstructionType::MOV, current_register, effective_register, IROperand::None());;
        register_vr_type(current_register, effective_register);
        emit_instruction(mov);
    }

}

void InstructionGenerator::visit_compound(uptr<compound_statement>& compound)
{
    current_scope = current_scope->get_child();
    for (auto& statement : compound->statements)
    {
        visit_statement(statement);
    }
    current_scope = current_scope->parent;
}

void InstructionGenerator::visit_expression_s(uptr<expression_statement>& expr)
{
    visit_expression(expr->expr);
}

void InstructionGenerator::visit_string_literal(uptr<string_literal_expr>& string_literal) 
{
    int64_t literal_location = (int64_t)ir_unit.allocate_string_literal(string_literal->value);
    IRInstruction load_string(IRInstructionType::LOAD_MEM_INDEX, current_register, IROperand::LiteralLocation(literal_location), IROperand::None());
    last_string_literal_location = load_string.src1.value;
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
    IRInstruction load_float(IRInstructionType::LOAD_FLOAT64, current_register, IROperand::LiteralLocation(literal_location), IROperand::None());
    register_vr_type(current_register, TYPE_F64);
    emit_instruction(load_float);
}

void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal)
{
    Symbol* symbol = current_scope->find_variable_symbol(identifier_literal);
    int64_t vr = symbol_id_to_vr.at(symbol->id);
    const Type& vr_type = get_vr_type(IROperand::Vr(vr));
    if (is_struct_type(vr_type)) {
        IRInstruction lea(IRInstructionType::LOAD_STRUCT, current_register, IROperand::Vr(vr), IROperand::None());
        register_vr_type(current_register, vr_type);
        emit_instruction(lea);
    } else if (is_array_type(vr_type)) {
        IRInstruction lea(IRInstructionType::LOAD_EFFECTIVE_ADDRESS, current_register, IROperand::Vr(vr), IROperand::None());
        register_vr_type(current_register, vr_type);
        emit_instruction(lea);
    }
    else
    {
        IRInstruction mov(IRInstructionType::MOV, current_register, IROperand::Vr(vr), IROperand::None());
        register_vr_type(current_register, mov.src1);
        emit_instruction(mov);
    }
}

void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    // todo check current function return type
    visit_expression(return_s->return_expression);
    
    if (current_function.symbol->type.byte_size() != 0)
        emit_implicit_conversion(*return_s, effective_register, current_function.symbol->type);
    IRInstruction return_inst(IRInstructionType::RETURN, current_register, effective_register, IROperand::None());
    register_vr_type(current_register, effective_register);
    
    emit_instruction(return_inst);
}

void InstructionGenerator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration)
{

    ExpectedSymbol expected = 
        current_scope->insert_variable_symbol(variable_declaration->var_name, variable_declaration->var_type);
    
    if (!expected.has_value()) {
        Logger::error(*variable_declaration, expected.error());
    }

    Symbol* symbol = expected.value();

    if (symbol->type.is_array()) {
        IRInstruction stack_allocation(IRInstructionType::RESERVE_STACK, 
                current_register, IROperand::Immediate((int64_t)symbol->type.array_byte_size()), IROperand::None());
        emit_instruction(stack_allocation, 0, false);
    }

    else if (is_struct_type(symbol->type)) {
        IRInstruction stack_allocation(IRInstructionType::RESERVE_STACK, 
                current_register, IROperand::Immediate((int64_t)symbol->type.byte_size()), IROperand::None());
        emit_instruction(stack_allocation, 0, false);
    }
    

    symbol_id_to_vr.emplace(symbol->id, current_register.value);
    register_vr_type(current_register, symbol->type);
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
    
    Symbol* symbol = current_scope->find_variable_symbol(variable_definition->declaration->var_name); 
    int64_t vr = symbol_id_to_vr.at(symbol->id);

    // used for #strlen
    if (std::holds_alternative<uptr<string_literal_expr>>(variable_definition->definition)) {
        identifier_string_literal_location.insert({vr, last_string_literal_location});
    }

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
        if (type_from.byte_size() >= type_to.byte_size()) return;
        if (is_unsigned(type_from)) return;

        IRInstruction sign_extend(IRInstructionType::SIGN_EXTEND, current_register, vr, IROperand::None());
        register_vr_type(current_register, type_to);
        emit_instruction(sign_extend);
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
            Symbol* symbol = current_scope->find_variable_symbol(identifier);

            emit_implicit_conversion(*identifier, right_register, symbol->type);
            int64_t vr = symbol_id_to_vr.at(symbol->id);
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
        [this, op_token, &right_side, &right_register](uptr<binop_expr>& binary)
        {
            if (binary->op_token != TokenType::DOT)
            {
                Logger::error(*binary, "Tried to assign a non-assignable value");
            }

            emit_struct_field_address(binary);
            IROperand left_side = effective_register;

            emit_implicit_conversion(*as_ast_node(right_side), right_register, *prevous_struct_access_type);


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
        emit_assignment(binop->op_token, binop->left, binop->right);
        return;
    }
    
    if (binop->op_token == TokenType::DOT) 
    {
        visit_struct_access(binop);
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
        right_register = effective_register;
    }

    IRInstruction mov_src1(IRInstructionType::MOV, current_register, left_register, IROperand::None()); 
    register_vr_type(current_register, left_register);
    emit_instruction(mov_src1);

    IRInstructionType instruction_type = get_ir_binop_instruction(binop->op_token, left_register);

    IRInstruction binop_inst(instruction_type, current_register, effective_register, right_register);

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
        if (std::holds_alternative<uptr<binop_expr>>(unary->expr))
        {
            auto& binop = std::get<uptr<binop_expr>>(unary->expr);
            if (binop->op_token != TokenType::DOT) {
                Logger::error(*unary, "Trying to get the address of a non-literal");
            }
            
            emit_struct_field_address(binop);
            return;
        }
        if (!std::holds_alternative<uptr<identifier_literal_expr>>(unary->expr))
        {
            Logger::error(*unary, "Trying to get the address of a non-literal");
        }
        auto& identifier_literal = std::get<uptr<identifier_literal_expr>>(unary->expr);
        Symbol* symbol = current_scope->find_variable_symbol(identifier_literal);

        int64_t vr = symbol_id_to_vr.at(symbol->id);
        IROperand var_vr = IROperand::Vr(vr);
        IRInstruction addr_of(IRInstructionType::ADDR_OF, current_register, var_vr, IROperand::None());
        register_vr_type(current_register, symbol->type.get_ptr());
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
    case TokenType::MINUS:
    {
        const auto expr_type = get_vr_type(effective_register);
        if (is_integer_type(expr_type)) {
            IRInstruction neg(IRInstructionType::INEG, current_register, effective_register, IROperand::None());
            register_vr_type(current_register, effective_register);
            emit_instruction(neg);
        } else if (is_float_type(expr_type)) {
            IRInstructionType inst_type;
            if (expr_type.byte_size() == 8) {
                inst_type = IRInstructionType::F64NEG;
            } else {
                inst_type = IRInstructionType::F32NEG;
            }

            IRInstruction neg(inst_type, current_register, effective_register, IROperand::None());

            register_vr_type(current_register, effective_register);
            emit_instruction(neg);
        } else {
            Logger::error(*unary, "Unary operator '-' not supported for this type");
        }
        break;
    }
    default:
        Logger::error(*unary, "Unknown unary operator");
    }

}

void InstructionGenerator::visit_explicit_conversion(uptr<explicit_conversion_expr>& explicit_conversion)
{
    visit_expression(explicit_conversion->expr);
    current_scope->resolve_type(explicit_conversion->type);
    // todo other ops than ptr cast
    auto effective_type = get_vr_type(effective_register);
    if (is_pointer_type(explicit_conversion->type) && is_pointer_type(effective_type)) {
        IRInstruction mov(IRInstructionType::MOV, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, explicit_conversion->type);
        emit_instruction(mov);
        return;
    }
    if (is_integer_type(explicit_conversion->type) && is_integer_type(effective_type)) {
        if (explicit_conversion->type.byte_size() <= effective_type.byte_size()) {
            IRInstruction mov(IRInstructionType::MOV, current_register, effective_register, IROperand::None());
            register_vr_type(current_register, explicit_conversion->type);
            emit_instruction(mov);
            return;
        }

        IRInstruction sign_extend(IRInstructionType::SIGN_EXTEND, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, explicit_conversion->type);
        emit_instruction(sign_extend);
        
        return;
    }
    if (is_float_type(explicit_conversion->type) && is_integer_type(effective_type)) {
        IRInstructionType conversion_type;
        switch (explicit_conversion->type.byte_size()) {
            case 8: // f64
                conversion_type = IRInstructionType::CONVERT_INT_TO_F64;
                break;
            case 4: // f32
                conversion_type = IRInstructionType::CONVERT_INT_TO_F32;
                break;
            default:
                Logger::error(*explicit_conversion, "unknown float size");
        }
        if (effective_type.byte_size() < 4) {
            Type implicit_type = TYPE_I32;
            if (is_unsigned(effective_type)) implicit_type = TYPE_U32;
            emit_implicit_conversion(*explicit_conversion, effective_register, implicit_type);
        }
        IRInstruction conversion(conversion_type, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, explicit_conversion->type);
        emit_instruction(conversion);        
        return;
    }
    if (is_integer_type(explicit_conversion->type) && is_float_type(effective_type)) {
        IRInstructionType conversion_type;
        switch (effective_type.byte_size()) {
            case 8: // f64
                conversion_type = IRInstructionType::CONVERT_F64_TO_INT;
                break;
            case 4: // f32
                conversion_type = IRInstructionType::CONVERT_F32_TO_INT;
                break;
            default:
                Logger::not_implemented(); //shouldnt get here;
        }
        IRInstruction conversion(conversion_type, current_register, effective_register, IROperand::None());
        register_vr_type(current_register, explicit_conversion->type);
        emit_instruction(conversion);        
        return;
    }

    if ((is_float_type(explicit_conversion->type) && is_float_type(effective_type)) ||
        (is_integer_type(explicit_conversion->type) && is_integer_type(effective_type)))
    {
        emit_implicit_conversion(*explicit_conversion, effective_register, explicit_conversion->type);
        return;
    }

    Logger::error(*explicit_conversion, "Illegal conversion");
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
    current_scope = current_scope->get_child();

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

    current_scope = current_scope->parent;
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

void InstructionGenerator::visit_macro_expr(uptr<macro_expr>& macro) 
{
    if (macro->command_name == "strlen") {
        std::visit(overloads{
            [&](uptr<string_literal_expr>& string_literal) {
                visit_string_literal(string_literal);
                int64_t strlen_id = (int64_t)ir_unit.allocate_stringlength_literal(last_string_literal_location);
                IRInstruction strlen(IRInstructionType::MOV, current_register, IROperand::Literal(strlen_id), IROperand::None());
                register_vr_type(current_register, TYPE_U64);
                emit_instruction(strlen); 
            },
            [&](uptr<identifier_literal_expr>& identifier) {
                Symbol* symbol = current_scope->find_variable_symbol(identifier);
                auto& type = symbol->type;
                if (!is_string(type)) Logger::error(*identifier, "Cannot process the string length for non-string-type"); 
                auto vr = symbol_id_to_vr.at(symbol->id);
                if (!identifier_string_literal_location.contains(vr)) 
                    Logger::error(*identifier, "Identifier is not a const string");
                
                int64_t string_id = identifier_string_literal_location.at(vr);
                int64_t strlen_id = (int64_t)ir_unit.allocate_stringlength_literal(string_id);
                IRInstruction strlen(IRInstructionType::MOV, current_register, IROperand::Literal(strlen_id), IROperand::None());
                register_vr_type(current_register, TYPE_U64);
                emit_instruction(strlen); 
            },

            [&, this](auto& expr) { Logger::error(*expr, "Cannot process the string length for node"); }
        }, macro->argument);

    }
    else if (macro->command_name == "sizeof") {
        uint32_t byte_size;
        // do this becuase we dont want to generate code for the identifier
        if (std::holds_alternative<uptr<identifier_literal_expr>>(macro->argument))
        {
            auto& identifier = std::get<uptr<identifier_literal_expr>>(macro->argument);
            Symbol* possible_symbol = current_scope->find_variable_symbol(identifier->name);
            if (!possible_symbol)
            {
                possible_symbol = current_scope->find_typedef_symbol(identifier->name);
            }
            if (!possible_symbol)
            {
                Logger::error(*identifier, std::format("'{}' not found", identifier->name));
            }

            if (is_array_type(possible_symbol->type))
                byte_size = possible_symbol->type.array_byte_size();
            else if (is_struct_type(possible_symbol->type))
                byte_size = possible_symbol->type.struct_byte_size();
            else
                byte_size = possible_symbol->type.byte_size();
        }
        else
        {
            visit_expression(macro->argument);
            auto type = get_vr_type(effective_register);
            if (is_array_type(type))
                byte_size = type.array_byte_size();
            else
                byte_size = type.byte_size();
        }

        IRInstruction load_int(IRInstructionType::LOAD_IMMEDIATE, current_register, IROperand::Immediate(byte_size), IROperand::None());
        register_vr_type(current_register, TYPE_U64);
        emit_instruction(load_int);
    }
    else {
        Logger::error(*macro, "Unknown macro command");
    }

}

void InstructionGenerator::visit_struct(uptr<struct_statement>& struct_s) 
{
    
   

    StructType st = push_user_defined_struct();
    Type type = Type::Struct(st);

    ExpectedSymbol expected = current_scope->insert_typedef_symbol(struct_s->name, type, 0);
    if (!expected.has_value()) {
        Logger::error(*struct_s, expected.error());
    }

    Symbol* symbol = expected.value();

    std::map<std::string, Type> fields;
    for (auto& field : struct_s->fields) {
        if (!current_scope->resolve_type(field->var_type))
        {
            Logger::error(*field, "Type not defined");
        }
        
        fields.emplace(field->var_name, field->var_type);
    }
    UserDefinedStruct& defined = get_user_defined_struct(st.id);
    defined.fields = fields;
    defined.calculate_offset();
    symbol->type.set_struct_byte_size(defined.byte_size);
}

void InstructionGenerator::visit_struct_access(uptr<binop_expr>& binop)
{
    uint32_t offset = get_struct_field_offset(binop);

    IRInstruction field_access(IRInstructionType::STRUCT_FIELD_ACCESS,
            current_register, effective_register, IROperand::Immediate(offset));

    if (!prevous_struct_access_type) Logger::internal_error();

    register_vr_type(current_register, *prevous_struct_access_type);
    emit_instruction(field_access);
}

void InstructionGenerator::emit_struct_field_address(uptr<binop_expr>& binop)
{
    
    uint32_t offset = get_struct_field_offset(binop);

    IRInstruction field_access(IRInstructionType::STRUCT_FIELD_ACCESS,
            current_register, effective_register, IROperand::Immediate(offset));

    if (!prevous_struct_access_type) Logger::internal_error();

    IRInstruction field_address(IRInstructionType::STRUCT_FIELD_ADDRESS,
            current_register, effective_register, IROperand::Immediate(offset));
    register_vr_type(current_register, prevous_struct_access_type->get_ptr());
    emit_instruction(field_address);
}

uint32_t InstructionGenerator::get_struct_field_offset(uptr<binop_expr>& binop)
{
    
    if (binop->op_token != TokenType::DOT)
    {
        Logger::error(*binop, "Unexpected expression");
    }
    
    expression& left = binop->left;
    expression& right = binop->right;

    if (!std::holds_alternative<uptr<identifier_literal_expr>>(right))
    {
        Logger::error(*as_ast_node(right), "non-identifier struct field");
    }

    uint32_t offset = 0; 

    auto& right_identifier = std::get<uptr<identifier_literal_expr>>(right);

    if (!std::holds_alternative<uptr<identifier_literal_expr>>(left))
    {
        if (std::holds_alternative<uptr<binop_expr>>(left))
        {
            auto& left_binop = std::get<uptr<binop_expr>>(left);
            offset += get_struct_field_offset(left_binop);
        }
        else
        {
            visit_expression(left);
            prevous_struct_access_type = &const_cast<Type&>(get_vr_type(effective_register));
        }
        
        // check this is not a nullptr in case
        if (!prevous_struct_access_type) Logger::internal_error();
         
        Type left_type = *prevous_struct_access_type;

        if (left_type.ptr_depth) {
            Logger::error(*as_ast_node(left), "Cannot access pointer-type");
        }
        if (!is_struct_type(left_type)) {
            Logger::error(*as_ast_node(left), "Cannot access non-struct-type");
        }

        StructType st = std::get<StructType>(left_type.base);

        UserDefinedStruct& struct_content = get_user_defined_struct(st.id);

        if (!struct_content.fields.contains(right_identifier->name))
        {
            Logger::error(*right_identifier, "Struct field not found");
        }

        Type& right_type = struct_content.fields.at(right_identifier->name);
        // set this for the next recursion step
        prevous_struct_access_type = &right_type;
        
        return right_type.offset + offset;
    }
    else
    {
        auto& left_identifier = std::get<uptr<identifier_literal_expr>>(left);
        Symbol* left_symbol = current_scope->find_variable_symbol(left_identifier);
        if (left_symbol->type.ptr_depth) {
            Logger::error(*left_identifier, "Cannot access pointer-type");
        }
        if (!is_struct_type(left_symbol->type)) {
            Logger::error(*left_identifier, "Cannot access non-struct-type");
        }
    
        StructType st = std::get<StructType>(left_symbol->type.base);
        auto left_vr = IROperand::Vr(symbol_id_to_vr.at(left_symbol->id));

        // this is important and needs to be preserved in this recursive function
        effective_register = left_vr;

        UserDefinedStruct& struct_content = get_user_defined_struct(st.id);
        if (!struct_content.fields.contains(right_identifier->name))
        {
            Logger::error(*right_identifier, "Struct field not found");
        }

        Type& right_type = struct_content.fields.at(right_identifier->name);

        // set this for the next recursion step
        prevous_struct_access_type = &right_type;
        
        return right_type.offset;
    }
}

} // namespace hx

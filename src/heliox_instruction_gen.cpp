#include "heliox_instruction_gen.hpp"
namespace hx
{

InstructionGenerator::InstructionGenerator(sptr<SymbolTable> global_table)
    : global_table(global_table) {}

void InstructionGenerator::visit_program(uptr<Program>& prog)
{
    for (auto& func : prog->functions)
    {
        global_table->add_function_symbol(
                func->identifier->name, func->type, func->get_parameter_type_data());
    }
    for (auto& func : prog->functions)
    {
        current_virtual_register = 0;
        effective_register = 0;
        std::println("-------{}-------", func->identifier->name);
        visit_function(func); 
    }

    calculate_live_ranges(); 
}

void InstructionGenerator::calculate_live_ranges()
{
    

    for (auto& func : instruction_data.instruction_functions)
    {
        int in_loop = 0;
        std::vector<uint32_t> loop_starts;
        for (auto& triplet : func.instruction_triplets)
        {
            uint32_t instruc_count = triplet.instruc_count;
            if (triplet.instruction == Instruction::WHILE)
            {
                in_loop++;
                loop_starts.push_back(instruc_count);
            }
            else if (triplet.instruction == Instruction::ENDWHILE)
            {
                in_loop--;
                loop_starts.pop_back();
            }
            


            if (triplet.dst != -1)                 
            {
                if (!func.live_ranges.contains(triplet.dst))
                {
                    func.live_ranges.insert({triplet.dst, LiveRange{instruc_count, instruc_count}});
                }
                else
                {
                    uint32_t& last_use = func.live_ranges.at(triplet.dst).last_use;
                    last_use = std::max(instruc_count, last_use);
                }
            }
            for (auto& item : triplet.items)
            {
                if (item.item_type != ItemType::VIRTUAL_REGISTER) continue;
                if (!func.live_ranges.contains(item.value))
                {
                    func.live_ranges.insert({item.value, LiveRange{instruc_count, instruc_count}});
                }
                else
                {
                    uint32_t& last_use = func.live_ranges.at(item.value).last_use;
                    last_use = std::max(instruc_count, last_use);
                }
            }

            if (in_loop)
            {
                for (auto& [vr, live_range] : func.live_ranges)
                {
                    // todo currently sets the live ranges of the inner loop to the end of the outer loop
                    // can do this better
                    const auto var_vrs = current_table->get_all_variable_virtual_registers();
                    if (!var_vrs.contains(vr)) continue;
                    if (!func.live_ranges.contains(vr)) continue;
                    if (func.live_ranges.at(vr).last_use < loop_starts[0]) continue;
                    uint32_t& last_use = func.live_ranges.at(vr).last_use;
                    func.live_ranges.at(vr).last_use = std::max(instruc_count, last_use);
                }
            }
        }
    }
     
    for (auto& func : instruction_data.instruction_functions) 
    {
        for (const auto& [vr, live_range] : func.live_ranges)
        {
            std::println("r{} [{} -> {}]", 
                    vr, live_range.first_use,
                    live_range.last_use);
        }
    }
    
}
    
void InstructionGenerator::emit_instruction(InstructionTriplet triplet, uint32_t increment)
{
    triplet.instruc_count = instruction_count;
    instruction_count ++;
    print_instruction(triplet);
    set_vr_reg_size(triplet.dst, triplet.reg_size);
    current_virtual_register += increment;
    instruction_data.instruction_functions.back().instruction_triplets.push_back(triplet);
}

void InstructionGenerator::visit_function(uptr<function>& func)
{
    if (func->is_extern) 
    {
        instruction_data.instruction_functions.push_back({func->identifier->name, true});
        return;
    }
    current_table = global_table->add_table().get();
    
    instruction_data.instruction_functions.push_back({func->identifier->name});


    int parameter_position = 0;
    for (auto& param : func->params)
    {
        ReservedRegister reg_pair;
        if (parameter_position < 6)
        {
           reg_pair.reg = g_register_data.register_passed_arguments[parameter_position]; 
        }
        else
        {
            reg_pair.on_stack = true;
            reg_pair.stack_position = 16 + (parameter_position-6) * 8;
        }
        RegisterSize reg_size = get_register_size(param->var_type.byte_size);
        InstructionTriplet triplet = 
            InstructionTriplet(Instruction::LOAD_PARAM, 
                    current_virtual_register,
                    {Item{ItemType::PARAMETER_INDEX, parameter_position}},
                    reg_size);
        reserve_register(current_virtual_register, reg_pair); 
        effective_register = current_virtual_register;
        effective_register_size = reg_size;
        emit_instruction(triplet);

        InstructionTriplet store = 
            InstructionTriplet(Instruction::STORE, 
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    reg_size);
        effective_register = current_virtual_register;
        emit_instruction(store);

        current_table->add_variable_symbol(param->var_identifier->name,
                param->var_type, effective_register, false);
                
        parameter_position++;
    }



    for (auto& stat : func->statements)
    {
        visit_statement(stat);
    }
}

void InstructionGenerator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::LOAD_INT, 
                current_virtual_register,
                {Item{ItemType::IMMEDIATE_VALUE, std::stol(int_literal->value)}},
                RegisterSize::BIT64);
    effective_register = current_virtual_register;
    effective_register_size = RegisterSize::BIT64;
    emit_instruction(triplet);

}
void InstructionGenerator::visit_string_literal(uptr<string_literal_expr>& string_literal)  
{
    uint32_t label = global_table->add_string(string_literal->value);
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::LOAD_STRING, 
                current_virtual_register,
                {Item{ItemType::STRINGTABLE_INDEX, label}},
                RegisterSize::BIT64);
    effective_register = current_virtual_register;
    effective_register_size = triplet.reg_size;
    emit_instruction(triplet);
}
void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) 
{
    
    VariableSymbol& sym = current_table->find_variable_symbol(identifier_literal->name);
    if (sym.is_parameter)
    {
        RegisterSize reg_size = get_register_size(sym.type_info.byte_size);
        InstructionTriplet triplet = 
            InstructionTriplet(Instruction::LOAD_PARAM, 
                    current_virtual_register,
                    {Item{ItemType::PARAMETER_INDEX, sym.vr}},
                    reg_size);

        effective_register = current_virtual_register;
        effective_register_size = reg_size;
        emit_instruction(triplet);

        sym.is_parameter = false;
        sym.vr = effective_register;

    }
    else
    {
        effective_register = sym.vr;
        effective_register_size = get_register_size(sym.type_info.byte_size);
    }

}
void InstructionGenerator::visit_binop(uptr<binop_expr>& binop)  
{ 
    //TODO CHECK OP ASSOCIATIVITY
    //TODO "SMARTER" MORE EFFECTIVE SYSTEM RATHER THAN ALWAYS MOV THE LEFT SIDE SO SHIT DONT BREAK
    visit_expression(binop->left);
    virtual_register left = effective_register;
    RegisterSize left_size = effective_register_size;
    uint32_t effective_label; 
    if (binop->op_token == TokenType::LOGICAL_AND)
    {
        effective_label = logical_and_label_id;
        InstructionTriplet and_left(Instruction::LOGICAL_AND_TEST_LEFT,
            -1,
            {Item{ItemType::VIRTUAL_REGISTER, left}, Item{ItemType::IMMEDIATE_VALUE, logical_and_label_id}},
            left_size);
        logical_and_label_id++;
        emit_instruction(and_left, 0);
    }
    else if (binop->op_token == TokenType::LOGICAL_OR)
    {
        effective_label = logical_or_label_id;
        InstructionTriplet or_left(Instruction::LOGICAL_OR_TEST_LEFT,
            -1,
            {Item{ItemType::VIRTUAL_REGISTER, left}, Item{ItemType::IMMEDIATE_VALUE, logical_or_label_id}},
            left_size);
        logical_or_label_id++;
        emit_instruction(or_left, 0);
    }

    visit_expression(binop->right);
    virtual_register right = effective_register;
    RegisterSize right_size = effective_register_size;
    

    if (left_size != right_size)
    {
        std::println("WARNING: Trying to do operations with 2 different operation sizes");
        left_size = std::min(left_size, right_size);
        right_size = left_size;
    }

    if (binop->op_token == TokenType::LOGICAL_AND)
    {
        
        InstructionTriplet and_right(Instruction::LOGICAL_AND_TEST_RIGHT,
            current_virtual_register,
            {Item{ItemType::VIRTUAL_REGISTER, right}, Item{ItemType::IMMEDIATE_VALUE, effective_label}},
            right_size);
        effective_register = current_virtual_register;
        emit_instruction(and_right);
        return;
    }
    else if (binop->op_token == TokenType::LOGICAL_OR)
    {
        
        InstructionTriplet or_right(Instruction::LOGICAL_OR_TEST_RIGHT,
            current_virtual_register,
            {Item{ItemType::VIRTUAL_REGISTER, right}, Item{ItemType::IMMEDIATE_VALUE, effective_label}},
            right_size);
        effective_register = current_virtual_register;
        emit_instruction(or_right);
        return;
    }
    
    Instruction instruc;
    bool is_equals_op = true;
    switch (binop->op_token)
    {
        case TokenType::EQU: 
            instruc = Instruction::STORE;
            break;
        case TokenType::PLUSEQUALS: 
            instruc = Instruction::ADD;
            break;
        case TokenType::MINUSEQUALS: 
            instruc = Instruction::SUB;
            break;
        case TokenType::MULEQUALS: 
            {
            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, left}},
                    left_size);
            // it can actually be any register but might implement later
            reserve_register(current_virtual_register, {Register::A});
            effective_register = current_virtual_register;
            emit_instruction(store);
            InstructionTriplet triplet = InstructionTriplet(
                Instruction::MUL,
                effective_register,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                left_size);
            emit_instruction(triplet, 0);
            InstructionTriplet store_back(Instruction::STORE,
                    left,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    left_size);
            emit_instruction(store_back, 0);
            effective_register = left;
            return;
            }
        case TokenType::DIVEQUALS:
            {
            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, left}},
                    left_size);

            ReservedRegister reserved_register; 
            reserved_register.reg = Register::A;
            reserved_register.reserved_without_vr.push_back(Register::D);
            reserve_register(current_virtual_register, reserved_register);
            effective_register = current_virtual_register;
            emit_instruction(store);
            InstructionTriplet triplet = InstructionTriplet(
                Instruction::DIV,
                effective_register,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                left_size);
            emit_instruction(triplet, 0);
            InstructionTriplet store_back(Instruction::STORE,
                    left,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    left_size);
            emit_instruction(store_back, 0);
            effective_register = left;

            return;
            }
        case TokenType::MODEQUALS:
            {
            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, left}},
                    left_size);

            reserve_register(current_virtual_register, {Register::A});
            effective_register = current_virtual_register;
            emit_instruction(store);

            InstructionTriplet triplet = InstructionTriplet(
                Instruction::MOD,
                current_virtual_register,
                {Item{ItemType::VIRTUAL_REGISTER, effective_register},
                 Item{ItemType::VIRTUAL_REGISTER, right}},
                left_size);
            reserve_register(current_virtual_register, {Register::D});
            effective_register = current_virtual_register;
            emit_instruction(triplet);
            InstructionTriplet store_back(Instruction::STORE,
                    left,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    left_size);
            emit_instruction(store_back, 0);
            effective_register = left;
            return;
            }
        default:
            is_equals_op = false;

    }
    if (is_equals_op)
    {
        InstructionTriplet triplet = InstructionTriplet(
            instruc,
            left,
            {Item{ItemType::VIRTUAL_REGISTER, right}},
            left_size);
        
        emit_instruction(triplet, 0);
        return;
    }


    InstructionTriplet left_side_triplet(
        Instruction::STORE,
        current_virtual_register,
        {Item{ItemType::VIRTUAL_REGISTER, left}},
        left_size);

    effective_register = current_virtual_register;
    emit_instruction(left_side_triplet);

    switch (binop->op_token)
    {
        case TokenType::PLUS:
            instruc = Instruction::ADD;
            break;
        case TokenType::MINUS:
            instruc = Instruction::SUB;
            break;
        case TokenType::MULTIPLY:
            {
            instruc = Instruction::MUL;
            ReservedRegister reserved_register;
            // it can actually be any register but might implement later
            reserved_register.reg = Register::A;
            reserve_register(effective_register, reserved_register);
            }
            break;
        case TokenType::DIVIDE:
            {
            instruc = Instruction::DIV;
            ReservedRegister reserved_register; 
            reserved_register.reg = Register::A;
            reserved_register.reserved_without_vr.push_back(Register::D);
            reserve_register(effective_register, reserved_register);
            break;
            }
        case TokenType::MODULO:
            {
            instruc = Instruction::MOD;
            reserve_register(effective_register, {Register::A});
            reserve_register(current_virtual_register, {Register::D});
            InstructionTriplet triplet = 
                InstructionTriplet(instruc, 
                        current_virtual_register,
                        {Item{ItemType::VIRTUAL_REGISTER, effective_register},
                        Item{ItemType::VIRTUAL_REGISTER, right}},
                        left_size);
            effective_register = current_virtual_register;
            emit_instruction(triplet);
            return;
            }
        case TokenType::DOUBLE_EQU:
            instruc = Instruction::IS_EQUAL;
            break;
        case TokenType::NEQU:
            instruc = Instruction::NOT_EQUAL;
            break;
        case TokenType::GT:
            instruc = Instruction::GREATER_THAN;
            break;
        case TokenType::GTE:
            instruc = Instruction::GREATER_OR_EQUAL_THAN;
            break;
        case TokenType::LT:
            instruc = Instruction::LESS_THAN;
            break;
        case TokenType::LTE:
            instruc = Instruction::LESS_OR_EQUAL_THAN;
            break;
        
        case TokenType::BITWISE_AND:
            instruc = Instruction::BITWISE_AND;
            break;
        case TokenType::BITWISE_OR:
            instruc = Instruction::BITWISE_OR;
            break;
        case TokenType::BITWISE_XOR:
            instruc = Instruction::BITWISE_XOR;
            break;

        default:
            //TODO IMPLEMENT MORE
            std::println("ERROR: UNKNOWN BINARY OPERATION");
            exit(-1);
    }
    InstructionTriplet triplet = 
        InstructionTriplet(instruc, 
                effective_register,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                left_size);
    emit_instruction(triplet, 0);

}
void InstructionGenerator::visit_unary(uptr<unary_expr>& unary)  
{
    Instruction instruction;
    switch (unary->op_token)
    {
        case TokenType::MINUS:
            instruction = Instruction::NEG;
            break;
        case TokenType::BITWISE_NOT:
            instruction = Instruction::BITWISE_NOT;
            break;
        case TokenType::NOT:
            instruction = Instruction::LOGICAL_NOT;
            break;
        case TokenType::MULTIPLY:
            {
            visit_expression(unary->expr);
             
            InstructionTriplet triplet(Instruction::DEREF,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    effective_register_size);
            effective_register = current_virtual_register;
            emit_instruction(triplet);
            return;
            }
        default:
            //TODO IMPLEMENT MORE
            std::println("ERROR: UNKNOWN UNARY OPERATION");
            exit(-1);
    }
    visit_expression(unary->expr);
    InstructionTriplet store(Instruction::STORE,
            current_virtual_register,
            {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
            effective_register_size);
    effective_register = current_virtual_register;
    emit_instruction(store);
    InstructionTriplet triplet(instruction,
            effective_register,
            {},
            effective_register_size);
    emit_instruction(triplet, 0);
}
void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call) 
{
    FunctionSymbol s = current_table->find_function_symbol(function_call->identifier->name);
    uint32_t label = s.id;
    
    std::vector<Item> parameter_virtual_registers = 
        {Item{ItemType::FUNCTIONTABLE_INDEX, label}};
    std::vector<RegisterSize> param_sizes = {RegisterSize::BIT64};

    std::vector<InstructionTriplet> push_param_triplets;
    for (int i = 0; i < function_call->parameters.size(); i++)
    {
        auto& param = function_call->parameters[i];
        visit_expression(param);
        
        if (i > 5)
        {
            InstructionTriplet triplet = 
                InstructionTriplet(Instruction::PUSH, 
                        effective_register,
                        {},
                        RegisterSize::BIT64);
            push_param_triplets.push_back(triplet);
        }
        param_sizes.push_back(effective_register_size);
        // save previous current_virtual_register 
        parameter_virtual_registers.push_back(
                Item{ItemType::VIRTUAL_REGISTER, effective_register}); 
    }

    // register passed args
    for (int i = 1; i < param_sizes.size(); i++)
    {
        if (i == 7) break;
        auto& item = parameter_virtual_registers[i];
        InstructionTriplet triplet(Instruction::STORE,
            current_virtual_register,
            {item},
            param_sizes[i]);
        ReservedRegister res;
        res.reg = g_register_data.register_passed_arguments[i-1];
        reserve_register(current_virtual_register, res);
        effective_register = current_virtual_register;
        emit_instruction(triplet);
        item.value = effective_register;
    }


    // align stack
    bool did_allignment = false;
    int pushed_param_count = function_call->parameters.size() - 6;
    if (function_call->parameters.size() > 6) 
    {
        if (pushed_param_count % 2 == 0)
        {
            InstructionTriplet align_triplet(
                Instruction::ALIGN,
                -1,
                {Item{ItemType::IMMEDIATE_VALUE, -8}},
                RegisterSize::BIT64);
            emit_instruction(align_triplet, 0);
            did_allignment = true;
        }
    }
    // push in reverse order
    for (int i = push_param_triplets.size()-1; i >= 0; i--) 
    {
        InstructionTriplet& triplet = push_param_triplets[i];
        InstructionTriplet store(Instruction::STORE,
                current_virtual_register,
                {Item{ItemType::VIRTUAL_REGISTER, triplet.dst}},
                param_sizes[i+1]);
        effective_register = current_virtual_register;
        emit_instruction(store);
        
        triplet.dst = effective_register;
        emit_instruction(triplet);
    }

    std::vector<ReservedRegister> reserved_registers;
    if (s.return_type.byte_size != 0)
    {
        reserve_register(current_virtual_register, {Register::A});
    }
    InstructionTriplet triplet(Instruction::CALL, 
                current_virtual_register,
                parameter_virtual_registers,
                get_register_size(s.return_type.byte_size));

    effective_register = current_virtual_register;
    emit_instruction(triplet);
    if (did_allignment)
    {
        InstructionTriplet align_triplet(
            Instruction::ALIGN,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, 8 + 8*pushed_param_count}},
            RegisterSize::BIT64);
        emit_instruction(align_triplet, 0);
    }
    
    if (s.return_type.byte_size != 0)
    {
        effective_register_size = get_register_size(s.return_type.byte_size);
        InstructionTriplet store(Instruction::STORE,
                current_virtual_register,
                {Item{ItemType::VIRTUAL_REGISTER, effective_register}}, effective_register_size);
        effective_register = current_virtual_register;
        emit_instruction(store);
    }
}

void InstructionGenerator::visit_compound(uptr<compound_statement>& compound) 
{
    current_table = current_table->add_table().get();
    for (auto& stat : compound->statements)
    {
        visit_statement(stat);
    }
    current_table = current_table->get_parent();
}
void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    visit_expression(return_s->return_expression);
    
    InstructionTriplet store(Instruction::STORE,
            current_virtual_register,
            {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
            effective_register_size);
    effective_register = current_virtual_register;
    emit_instruction(store);


    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::RETURN, 
                 effective_register,
                {},
                effective_register_size);
    reserve_register(effective_register, {Register::A});
    emit_instruction(triplet, 0);

}
void InstructionGenerator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) 
{
    current_table->add_variable_symbol(
            variable_declaration->var_identifier->name,
            variable_declaration->var_type, current_virtual_register);
}
void InstructionGenerator::visit_variable_definition(uptr<variable_definition_statement>& variable_definition) 
{
    visit_variable_declaration(variable_definition->declaration);

    VariableSymbol& sym = current_table->find_variable_symbol(
            variable_definition->declaration->var_identifier->name);
    
    effective_register_size = get_register_size(sym.type_info.byte_size);

    visit_expression(variable_definition->definition);
    sym.vr = current_virtual_register;

    RegisterSize expr_reg_size = effective_register_size;
    RegisterSize reg_size = get_register_size(sym.type_info.byte_size);
    if (expr_reg_size != reg_size)
    {
       // std::println("REG SIZE DOESNT MATCH FOR EXPR FOR SYMBOL {}",
       //         variable_definition->declaration->var_identifier->name);
       // exit(-1);
    }
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::STORE, 
                           current_virtual_register,
                           {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                           reg_size);
    
    effective_register = current_virtual_register;
    emit_instruction(triplet);
}
void InstructionGenerator::visit_conditional(uptr<conditional_statement>& conditional) 
{
    visit_expression(conditional->condition);
    // label
    InstructionTriplet if_begin(
        Instruction::IF,
        -1,
        {Item{ItemType::IMMEDIATE_VALUE, if_label_id}, Item{ItemType::VIRTUAL_REGISTER, effective_register}},
        effective_register_size);
    uint32_t effective_label = if_label_id;
    if_label_id++;

    emit_instruction(if_begin, 0);     

    visit_statement(conditional->then_stat);
    
    InstructionTriplet else_begin(
        Instruction::ELSE,
        -1,
        {Item{ItemType::IMMEDIATE_VALUE, effective_label}},
        RegisterSize::BIT0);

    emit_instruction(else_begin, 0);     
    // label
    visit_statement(conditional->else_stat);

    InstructionTriplet end_if(
        Instruction::ENDIF,
        -1,
        {Item{ItemType::IMMEDIATE_VALUE, effective_label}},
        RegisterSize::BIT0);

    emit_instruction(end_if, 0);     
    if_label_id++;
}
void InstructionGenerator::visit_while(uptr<while_statement>& while_s) 
{
    InstructionTriplet while_begin(Instruction::WHILE,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, while_label_id}},
            RegisterSize::BIT0);
    uint32_t effective_label_id = while_label_id;
    while_label_id++;
    emit_instruction(while_begin, 0);
    visit_expression(while_s->condition);
    InstructionTriplet jump(Instruction::WHILE_JUMPEND,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, effective_label_id},
             Item{ItemType::VIRTUAL_REGISTER, effective_register}},
            effective_register_size);
    emit_instruction(jump, 0);

    visit_statement(while_s->loop);
    InstructionTriplet while_end(Instruction::ENDWHILE,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, effective_label_id}},
            RegisterSize::BIT0);
    emit_instruction(while_end, 0);
}
void InstructionGenerator::visit_expression_s(uptr<expression_statement>& expr) 
{
    visit_expression(expr->expr);
}
void InstructionGenerator::visit_noop(uptr<noop_statement>& noop) 
{
} 

void InstructionGenerator::reserve_register(virtual_register vr, ReservedRegister reservation)
{
    instruction_data.instruction_functions.back().reserved_registers.insert({vr, reservation});
}
void InstructionGenerator::set_vr_reg_size(virtual_register vr, RegisterSize reg_size)
{
    instruction_data.instruction_functions.back().vr_reg_sizes.insert({vr, reg_size});
}

}

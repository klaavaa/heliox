#include "heliox_instruction_gen.hpp"
namespace hx
{

InstructionGenerator::InstructionGenerator(sptr<SymbolTable> global_table)
    : global_table(global_table), effective_type({primitive_type::VOID, 0}) {}

void InstructionGenerator::visit_program(uptr<Program>& prog)
{
    //for (auto& func : prog->functions)
    //{
    //    std::string function_name;
    //    if (!func->in_module.empty())
    //        function_name = func->in_module + "." + func->identifier->name;
    //    else 
    //        function_name = func->identifier->name;
    //    global_table->add_function_symbol(
    //            function_name, func->type, func->get_parameter_type_data(), func->has_varargs);
    //}
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
    if (triplet.dst != -1)
        set_vr_type(triplet.dst, triplet.type);
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
    // declaration
    if (func->statements.empty())
    {

        if (func->in_module.empty())
            instruction_data.instruction_functions.push_back({func->identifier->name, true});
        else 
            instruction_data.instruction_functions.push_back({func->in_module + "." + func->identifier->name, true});
        return; 
    }
    current_table = global_table->add_table().get();
    
    if (func->in_module.empty())
        instruction_data.instruction_functions.push_back({func->identifier->name});
    else 
        instruction_data.instruction_functions.push_back({func->in_module + "." + func->identifier->name});


    int32_t parameter_position = 0;
    uint32_t int_arg_count = 0;
    uint32_t float_arg_count = 0;
    uint32_t spilled_arg_count = 0;
    for (auto& param : func->params)
    {
        ReservedRegister reg_pair;
        if (is_float_type(param->var_type))
        {
        #ifdef _WIN32
            if (parameter_position < g_register_data.register_passed_float_args.size())
        #endif
        #ifdef __linux__
            if (float_arg_count < g_register_data.register_passed_float_args.size())
        #endif
            {
               reg_pair.reg = g_register_data.register_passed_float_args[float_arg_count++]; 
            }
            else
            {
                reg_pair.on_stack = true;
                reg_pair.stack_position = 16 + (spilled_arg_count++) * 8;
            }
        }
        else
        {
        #ifdef _WIN32
            if (parameter_position < g_register_data.register_passed_int_args.size())
        #endif
        #ifdef __linux__
            if (int_arg_count < g_register_data.register_passed_int_args.size())
        #endif
            {
               reg_pair.reg = g_register_data.register_passed_int_args[int_arg_count++]; 
            }
            else
            {
                reg_pair.on_stack = true;
                #ifdef _WIN32
                // windows "shadow space"
                reg_pair.stack_position = 32 + 16 + (parameter_position-4) * 8;
                #else
                reg_pair.stack_position = 16 + (spilled_arg_count++) * 8;
                #endif
            }
        }
        RegisterSize reg_size = get_register_size(param->var_type.byte_size);
        InstructionTriplet triplet = 
            InstructionTriplet(Instruction::LOAD_PARAM, 
                    current_virtual_register,
                    {Item{ItemType::PARAMETER_INDEX, parameter_position}},
                    param->var_type);
        reserve_register(current_virtual_register, reg_pair); 
        effective_register = current_virtual_register;
        effective_type = param->var_type;
        emit_instruction(triplet);

        InstructionTriplet store = 
            InstructionTriplet(Instruction::STORE, 
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    param->var_type);
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
                type_data{primitive_type::I64, 0});
    effective_register = current_virtual_register;
    effective_type = triplet.type;
    emit_instruction(triplet);

}
void InstructionGenerator::visit_float_literal(uptr<float_literal_expr>& float_literal) 
{
    uint32_t label = global_table->add_float(float_literal->value);
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::LOAD_FLOAT, 
                current_virtual_register,
                {Item{ItemType::FLOATTABLE_INDEX, label}},
                type_data{primitive_type::F64, 0});
    effective_register = current_virtual_register;
    effective_type = triplet.type;
    emit_instruction(triplet);
}

void InstructionGenerator::visit_string_literal(uptr<string_literal_expr>& string_literal)  
{
    uint32_t label = global_table->add_string(string_literal->value);
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::LOAD_STRING, 
                current_virtual_register,
                {Item{ItemType::STRINGTABLE_INDEX, label}},
                type_data{primitive_type::U8, 1});
    effective_register = current_virtual_register;
    effective_type = triplet.type;
    emit_instruction(triplet);
}
void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) 
{
    
    VariableSymbol& sym = current_table->find_variable_symbol(identifier_literal->name);
    if (sym.is_parameter)
    {
        InstructionTriplet triplet = 
            InstructionTriplet(Instruction::LOAD_PARAM, 
                    current_virtual_register,
                    {Item{ItemType::PARAMETER_INDEX, sym.vr}},
                    sym.type_info);

        effective_register = current_virtual_register;
        effective_type = sym.type_info;
        emit_instruction(triplet);

        sym.is_parameter = false;
        sym.vr = effective_register;

    }
    else
    {
        effective_register = sym.vr;
        effective_type = sym.type_info;
    }

}
void InstructionGenerator::visit_binop(uptr<binop_expr>& binop)  
{ 
    //TODO CHECK OP ASSOCIATIVITY
    //TODO "SMARTER" MORE EFFECTIVE SYSTEM RATHER THAN ALWAYS MOV THE LEFT SIDE SO SHIT DONT BREAK
    visit_expression(binop->left);
    virtual_register left = effective_register;
    type_data left_type = effective_type;
    uint32_t effective_label; 
    if (binop->op_token == TokenType::LOGICAL_AND)
    {
        effective_label = logical_and_label_id;
        InstructionTriplet and_left(Instruction::LOGICAL_AND_TEST_LEFT,
            -1,
            {Item{ItemType::VIRTUAL_REGISTER, left}, Item{ItemType::IMMEDIATE_VALUE, logical_and_label_id}},
            left_type);
        logical_and_label_id++;
        emit_instruction(and_left, 0);
    }
    else if (binop->op_token == TokenType::LOGICAL_OR)
    {
        effective_label = logical_or_label_id;
        InstructionTriplet or_left(Instruction::LOGICAL_OR_TEST_LEFT,
            -1,
            {Item{ItemType::VIRTUAL_REGISTER, left}, Item{ItemType::IMMEDIATE_VALUE, logical_or_label_id}},
            left_type);
        logical_or_label_id++;
        emit_instruction(or_left, 0);
    }

    visit_expression(binop->right);
    virtual_register right = effective_register;
    type_data right_type = effective_type;
    

    if (binop->op_token == TokenType::LOGICAL_AND)
    {
        
        InstructionTriplet and_right(Instruction::LOGICAL_AND_TEST_RIGHT,
            current_virtual_register,
            {Item{ItemType::VIRTUAL_REGISTER, right}, Item{ItemType::IMMEDIATE_VALUE, effective_label}},
            right_type);
        effective_register = current_virtual_register;
        emit_instruction(and_right);
        return;
    }
    else if (binop->op_token == TokenType::LOGICAL_OR)
    {
        
        InstructionTriplet or_right(Instruction::LOGICAL_OR_TEST_RIGHT,
            current_virtual_register,
            {Item{ItemType::VIRTUAL_REGISTER, right}, Item{ItemType::IMMEDIATE_VALUE, effective_label}},
            right_type);
        effective_register = current_virtual_register;
        emit_instruction(or_right);
        return;
    }
    
    if (left_type != right_type)
    {
        std::println("WARNING: Trying to do operations with 2 different operation sizes");
        implicit_convert(right, left_type, right_type);
        right = effective_register; 
        right_type = left_type;
    }

    Instruction instruc;
    bool is_equals_op = true;
    bool is_float = is_float_type(left_type);

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
            if (is_float)
            {
                instruc = Instruction::MUL;
                break;
            }
            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, left}},
                    left_type);
            // it can actually be any register but might implement later
            reserve_register(current_virtual_register, {Register::A});
            effective_register = current_virtual_register;
            emit_instruction(store);
            InstructionTriplet triplet = InstructionTriplet(
                Instruction::MUL,
                effective_register,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                left_type);
            emit_instruction(triplet, 0);
            InstructionTriplet store_back(Instruction::STORE,
                    left,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    left_type);
            emit_instruction(store_back, 0);
            effective_register = left;
            return;
            }
        case TokenType::DIVEQUALS:
            {
            if (is_float)
            {
                instruc = Instruction::DIV;
                break;
            }
            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, left}},
                    left_type);

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
                left_type);
            emit_instruction(triplet, 0);
            InstructionTriplet store_back(Instruction::STORE,
                    left,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    left_type);
            emit_instruction(store_back, 0);
            effective_register = left;

            return;
            }
        case TokenType::MODEQUALS:
            {
            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, left}},
                    left_type);

            reserve_register(current_virtual_register, {Register::A});
            effective_register = current_virtual_register;
            emit_instruction(store);

            InstructionTriplet triplet = InstructionTriplet(
                Instruction::MOD,
                current_virtual_register,
                {Item{ItemType::VIRTUAL_REGISTER, effective_register},
                 Item{ItemType::VIRTUAL_REGISTER, right}},
                left_type);
            reserve_register(current_virtual_register, {Register::D});
            effective_register = current_virtual_register;
            emit_instruction(triplet);
            InstructionTriplet store_back(Instruction::STORE,
                    left,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    left_type);
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
            left_type);
        
        emit_instruction(triplet, 0);
        return;
    }


    InstructionTriplet left_side_triplet(
        Instruction::STORE,
        current_virtual_register,
        {Item{ItemType::VIRTUAL_REGISTER, left}},
        left_type);
    
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
            if (is_float) break;
            ReservedRegister reserved_register;
            // it can actually be any register but might implement later
            reserved_register.reg = Register::A;
            reserve_register(effective_register, reserved_register);
            }
            break;
        case TokenType::DIVIDE:
            {
            instruc = Instruction::DIV;
            if (is_float) break;
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
                        left_type);
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
        case TokenType::SHIFT_LEFT:
            {
            instruc = Instruction::SHIFT_LEFT;
            ReservedRegister res;
            res.reg = Register::C;
            reserve_register(right, res);
            break;
            }
        case TokenType::SHIFT_RIGHT:
            {
            instruc = Instruction::SHIFT_RIGHT;
            ReservedRegister res;
            res.reg = Register::C;
            reserve_register(right, res);
            break;
            }

        default:
            //TODO IMPLEMENT MORE
            std::println("ERROR: UNKNOWN BINARY OPERATION");
            exit(-1);
    }
    InstructionTriplet triplet = 
        InstructionTriplet(instruc, 
                effective_register,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                left_type);
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
            type_data deref_type = effective_type;
            deref_type.ptr_depth--;
            InstructionTriplet triplet(Instruction::DEREF,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    deref_type);
            effective_register = current_virtual_register;
            effective_type = deref_type;
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
            effective_type);
    effective_register = current_virtual_register;
    emit_instruction(store);
    InstructionTriplet triplet(instruction,
            effective_register,
            {},
            effective_type);
    emit_instruction(triplet, 0);
}
void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call) 
{
    std::string func_name = function_call->identifier->name;
    if (!function_call->in_module.empty())
    {
        if (current_table->function_symbol_in_module(function_call->in_module, function_call->identifier->name))
            func_name = function_call->in_module + "." + function_call->identifier->name;
    }
    FunctionSymbol s = current_table->find_function_symbol(func_name);

    uint32_t label = s.id;
    
    std::vector<Item> parameter_virtual_registers = 
        {Item{ItemType::FUNCTIONTABLE_INDEX, label}};
    std::vector<type_data> param_types = {{primitive_type::VOID, 0}};

    std::vector<InstructionTriplet> push_param_triplets;
    uint32_t int_param_count = 0;
    uint32_t float_param_count = 0;
    // todo redo this whole bit
    for (int i = 0; i < function_call->parameters.size(); i++)
    {
        auto& param = function_call->parameters[i];
        visit_expression(param);
        bool pushed_to_stack = false; 
        if (is_float_type(effective_type))
        {
            // the windows x64 calling convention works differently than system V, so nth arg is always the nth register
        #ifdef _WIN32
            pushed_to_stack = i > g_register_data.register_passed_float_args.size() - 1;
        #endif
        #ifdef __linux__
            pushed_to_stack = float_param_count > g_register_data.register_passed_float_args.size() - 1;
        #endif
            float_param_count++;
        }
        else
        {
        #ifdef _WIN32
            pushed_to_stack = i > g_register_data.register_passed_int_args.size() - 1;
        #endif
        #ifdef __linux__
            pushed_to_stack = int_param_count > g_register_data.register_passed_int_args.size() - 1;
        #endif 
            int_param_count++;
        }

        if (pushed_to_stack)
        {
                InstructionTriplet triplet = 
                    InstructionTriplet(Instruction::PUSH, 
                            effective_register,
                            {},
                            {primitive_type::I64, 0});
                push_param_triplets.push_back(triplet);
        }

        // todo maybe change this
        if (i < s.parameter_types.size())
        {
            if (effective_type != s.parameter_types[i])
            {
                std::println("WARNING: expression type is different from functions parameter in call");
                implicit_convert(effective_register, s.parameter_types[i], effective_type);
            }
        }
        else if (!s.has_varargs)
        {
             
            std::println("ERROR: too many arguments passed in function call");
            exit(-1);
        }
        else
        {
            // it is a varg, meaning we have to convert floats up
            if (is_float_type(effective_type) && effective_type.byte_size == 4) 
            {
                implicit_convert(effective_register, {primitive_type::F64, 0}, effective_type);
            }
        }

        // todo maybe change this
        param_types.push_back(effective_type);
        // save previous current_virtual_register 
        parameter_virtual_registers.push_back(
                Item{ItemType::VIRTUAL_REGISTER, effective_register}); 
    }

    // register passed args
    int_param_count = 0;
    float_param_count = 0;
    for (int i = 1; i < param_types.size(); i++)
    {
        #ifdef _WIN32
        // basically check if the arg count is > 4
        if (i > g_register_data.register_passed_int_args.size()) continue;
        #endif

        #ifdef __linux__
        if (is_float_type(param_types[i])) 
        {
            if (float_param_count == g_register_data.register_passed_float_args.size()) continue;
        }
        {
            if (int_param_count == g_register_data.register_passed_int_args.size()) continue;
        }
        #endif
        auto& item = parameter_virtual_registers[i];
        InstructionTriplet triplet(Instruction::STORE,
            current_virtual_register,
            {item},
            param_types[i]);
        ReservedRegister res;
        if (is_float_type(param_types[i])) 
        {
            #ifdef _WIN32
            res.reg = g_register_data.register_passed_float_args[i - 1];
            #endif
            #ifdef __linux__
            res.reg = g_register_data.register_passed_float_args[float_param_count++];
            #endif
        }
        else
        {
            #ifdef _WIN32
            res.reg = g_register_data.register_passed_int_args[i - 1];
            #endif
            #ifdef __linux__
            res.reg = g_register_data.register_passed_int_args[int_param_count++];
            #endif
        }
        reserve_register(current_virtual_register, res);
        effective_register = current_virtual_register;
        emit_instruction(triplet);
        item.value = effective_register;
    }

    // save caller
    /*
    InstructionTriplet save_caller(Instruction::CALL_BEGIN,
            -1,
            {},
            {primitive_type::VOID, 0});
    emit_instruction(save_caller, 0);
    */

    // align stack
    bool did_allignment = false;
    uint32_t pushed_param_count = push_param_triplets.size();
    if (pushed_param_count && pushed_param_count % 2 == 0)
    {
        InstructionTriplet align_triplet(
            Instruction::ALIGN,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, -8}},
            {primitive_type::VOID, 0});
        emit_instruction(align_triplet, 0);
        did_allignment = true;
    }

    // push in reverse order
    for (uint32_t i = 0; i < push_param_triplets.size(); i++) 
    {
        InstructionTriplet& triplet = push_param_triplets[push_param_triplets.size() - 1 - i];
        InstructionTriplet store(Instruction::STORE,
                current_virtual_register,
                {Item{ItemType::VIRTUAL_REGISTER, triplet.dst}},
                param_types[param_types.size() - 1 - i]);
        effective_register = current_virtual_register;
        emit_instruction(store);
        
        triplet.dst = effective_register;
        emit_instruction(triplet);
    }
    
    if (s.has_varargs)
    {
        uint32_t vararg_count = function_call->parameters.size() - s.parameter_types.size();
        InstructionTriplet triplet(Instruction::LOAD_INT,
                current_virtual_register,
                {Item{ItemType::IMMEDIATE_VALUE, vararg_count}},
                {primitive_type::I64, 0});
        reserve_register(current_virtual_register, {Register::A}); 
        emit_instruction(triplet);
    }

    std::vector<ReservedRegister> reserved_registers;
    ReservedRegister reservation;
    reservation.reg = Register::A;
    if (s.return_type.byte_size != 0)
    {
        if (is_float_type(s.return_type))
            reservation.reg = Register::XMM0;
        else
            reservation.reg = Register::A;
    }
    reservation.reserved_without_vr = g_register_data.caller_saved_registers.get_available_registers();
    InstructionTriplet triplet(Instruction::CALL, 
                current_virtual_register,
                parameter_virtual_registers,
                s.return_type);
    reserve_register(triplet.dst, reservation);
    effective_register = current_virtual_register;
    emit_instruction(triplet);
    if (did_allignment)
    {
        InstructionTriplet align_triplet(
            Instruction::ALIGN,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, 8}},
            {primitive_type::VOID, 0});
        emit_instruction(align_triplet, 0);
    }
    if (pushed_param_count)
    {
        InstructionTriplet align_triplet(
            Instruction::ALIGN,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, 8*pushed_param_count}},
            {primitive_type::VOID, 0});
        emit_instruction(align_triplet, 0);
    }
    
    // load caller
    /*
    InstructionTriplet load_caller(Instruction::CALL_END,
            -1,
            {},
            {primitive_type::VOID, 0});
    emit_instruction(load_caller, 0);
    */

    if (s.return_type.byte_size != 0)
    {
        InstructionTriplet store(Instruction::STORE,
                current_virtual_register,
                {Item{ItemType::VIRTUAL_REGISTER, effective_register}}, 
                s.return_type);
        effective_type = s.return_type; 
        effective_register = current_virtual_register;
        emit_instruction(store);
    }

}


void InstructionGenerator::visit_explicit_conversion(uptr<explicit_conversion_expr>& explicit_conversion)
{
    visit_expression(explicit_conversion->expr);
    type_data from = effective_type;  

    if (is_float_type(explicit_conversion->type_info))
    {
        if (explicit_conversion->type_info.byte_size == 8)
        {
            Instruction conversion_type;
            if (is_float_type(from))
            {
                if (from.byte_size == 8) return;
                conversion_type = Instruction::CONVERTF32TOF64;
            }
            else if (is_unsigned(from))
            {
                //TODO
                conversion_type = Instruction::CONVERTU64TOF64; 
            }

            else
            {
                switch (from.byte_size)
                {
                case 8:
                    break;
                case 4:
                case 2: 
                case 1:
                    {
                    InstructionTriplet triplet(Instruction::SIGNEXTENDTOI64,
                            current_virtual_register,
                            {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                            {primitive_type::I64, 0}
                            );
                    effective_register = current_virtual_register;
                    effective_type = triplet.type;
                    emit_instruction(triplet);
                    break;
                    }
                case 0:
                    std::println("ERROR: UNKNOWN EXPLICIT CONVERSION");   
                    exit(-1);
                }

                conversion_type = Instruction::CONVERTI64TOF64;
            }
            InstructionTriplet triplet(conversion_type,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    {primitive_type::F64, 0}
                    );
            effective_register = current_virtual_register;
            effective_type = triplet.type;
            emit_instruction(triplet);
        }
        else if (explicit_conversion->type_info.byte_size == 4)
        {
            Instruction conversion_type;
            if (is_float_type(from))
            {
                if (from.byte_size == 4) return;
                conversion_type = Instruction::CONVERTF64TOF32;
            }
            else 
            {
                conversion_type = Instruction::CONVERTI64TOF32;
            }

            InstructionTriplet triplet(conversion_type,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    {primitive_type::F32, 0}
                    );
            effective_register = current_virtual_register;
            effective_type = triplet.type;
            emit_instruction(triplet);
        }
        else 
        {
            std::println("ERROR: UNKNOWN EXPLICIT CONVERSION");   
            exit(-1);
        }
    }
    else 
    {
        // cvtsi2ss
        // cvtsi2sd
        // vcvtusi2sd
        // vcvtusi2ss
        std::println("EXPLICIT CONVERSION NOT DEFINED YET");   
        exit(-1);
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
            effective_type);
    effective_register = current_virtual_register;
    emit_instruction(store);


    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::RETURN, 
                 effective_register,
                {},
                effective_type);
    if (is_float_type(effective_type))
        reserve_register(effective_register, {Register::XMM0});
    else 
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
    
    visit_expression(variable_definition->definition);
    if (sym.type_info != effective_type)
    {
        std::println("Warning: type doesnt match the expected type for variable: {}",
                variable_definition->declaration->var_identifier->name);
        implicit_convert(effective_register, sym.type_info, effective_type);
    }
    sym.vr = current_virtual_register;
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::STORE, 
                           current_virtual_register,
                           {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                           sym.type_info);
    

    effective_register = current_virtual_register;
    effective_type = sym.type_info;
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
        effective_type);
    uint32_t effective_label = if_label_id;
    if_label_id++;

    emit_instruction(if_begin, 0);     

    visit_statement(conditional->then_stat);
    
    InstructionTriplet else_begin(
        Instruction::ELSE,
        -1,
        {Item{ItemType::IMMEDIATE_VALUE, effective_label}},
        {primitive_type::VOID, 0});

    emit_instruction(else_begin, 0);     
    // label
    visit_statement(conditional->else_stat);

    InstructionTriplet end_if(
        Instruction::ENDIF,
        -1,
        {Item{ItemType::IMMEDIATE_VALUE, effective_label}},
        {primitive_type::VOID, 0});

    emit_instruction(end_if, 0);     
    if_label_id++;
}
void InstructionGenerator::visit_while(uptr<while_statement>& while_s) 
{
    InstructionTriplet while_begin(Instruction::WHILE,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, while_label_id}},
            {primitive_type::VOID, 0});
    uint32_t effective_label_id = while_label_id;
    while_label_id++;
    emit_instruction(while_begin, 0);
    visit_expression(while_s->condition);
    InstructionTriplet jump(Instruction::WHILE_JUMPEND,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, effective_label_id},
             Item{ItemType::VIRTUAL_REGISTER, effective_register}},
            effective_type);
    emit_instruction(jump, 0);

    visit_statement(while_s->loop);
    InstructionTriplet while_end(Instruction::ENDWHILE,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, effective_label_id}},
            {primitive_type::VOID, 0});
    emit_instruction(while_end, 0);
}
void InstructionGenerator::visit_expression_s(uptr<expression_statement>& expr) 
{
    visit_expression(expr->expr);
}
void InstructionGenerator::visit_noop(uptr<noop_statement>& noop) 
{
} 

void InstructionGenerator::visit_module(uptr<module_statement>& module_s)
{
    
}

void InstructionGenerator::visit_import(uptr<import_statement>& import_s) 
{

}

void InstructionGenerator::reserve_register(virtual_register vr, ReservedRegister reservation)
{
    instruction_data.instruction_functions.back().reserved_registers.insert({vr, reservation});
}
void InstructionGenerator::set_vr_type(virtual_register vr, type_data type)
{
    instruction_data.instruction_functions.back().vr_types.insert({vr, type});
}

void InstructionGenerator::implicit_convert(virtual_register vr, type_data wanted, type_data from)
{
    if (is_integer_type(from) && !is_integer_type(wanted) || 
        is_float_type(from) && !is_float_type(wanted))
    {
        std::println("Error: Implicit conversion not supported between different primitive types");    
        exit(-1);
    }


    if (is_integer_type(wanted)) return;
    if (is_float_type(wanted))
    {
        if (wanted.byte_size == 8) 
        {
            InstructionTriplet triplet(Instruction::CONVERTF32TOF64,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, vr}},
                    {primitive_type::F64, 0}
                    );
            effective_register = current_virtual_register;
            effective_type = triplet.type;
            emit_instruction(triplet);

            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    {primitive_type::F64, 0}
                    );
            effective_register = current_virtual_register;
            emit_instruction(store);
            return;
        }
        if (wanted.byte_size == 4)
        {
            InstructionTriplet triplet(Instruction::CONVERTF64TOF32,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, vr}},
                    {primitive_type::F32, 0}
                    );
            effective_register = current_virtual_register;
            effective_type = triplet.type;
            emit_instruction(triplet);

            InstructionTriplet store(Instruction::STORE,
                    current_virtual_register,
                    {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                    {primitive_type::F32, 0}
                    );
            effective_register = current_virtual_register;
            emit_instruction(store);
            return;
        }
    }
    std::println("ERROR: Implicit conversion with unknown type ({})", vr);
    exit(-1);

}
}

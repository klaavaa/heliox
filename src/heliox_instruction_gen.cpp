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
        std::println("-------{}-------", func->identifier->name);
        visit_function(func); 
    }

    calculate_live_ranges(); 
}

void InstructionGenerator::calculate_live_ranges()
{
    
    uint32_t instruc_count = 0;
    for (auto& func : instruction_data.instruction_functions)
    {
        for (auto& triplet : func.instruction_triplets)
        {
            switch (triplet.instruction)
            {
            case Instruction::DIV:
               // TODO: div idiv, unsigned vs signed division
                // TODO: xor rdx, rdx
                // reserves registers RAX, RDX, output in RAX
                break;
            case Instruction::CALL:
                // reservers registers RDI, RSI, RCX, RDX, R8, R9
                // output in RAX
                 
                break;

            default:
                break;
            }
            std::vector<virtual_register> used_registers;
            used_registers.push_back(triplet.dst);
            for (auto& i : triplet.items)
            {
               if(i.item_type == ItemType::VIRTUAL_REGISTER)
               {
                    used_registers.push_back(i.value);
               }
            }
            
            if (triplet.dst >= instruction_data.live_ranges.size())
                instruction_data.live_ranges.push_back(LiveRange{triplet.dst, triplet.reg_size, instruc_count, 0});

            for (auto vreg : used_registers)
            {
                if (vreg < instruction_data.live_ranges.size() )
                {
                    instruction_data.live_ranges[vreg].last_use = instruc_count; 
                }
            }
            instruc_count++;

        }
    }
    

    for (size_t i = 0; i < instruction_data.live_ranges.size(); i++)
    {
        std::println("r{} [{} -> {}]", i, instruction_data.live_ranges[i].first_use,
                instruction_data.live_ranges[i].last_use);
    }
}
    
void InstructionGenerator::emit_instruction(InstructionTriplet triplet, uint32_t increment)
{
    triplet.instruc_count = instruction_count;
    instruction_count ++;
    print_instruction(triplet);
    current_virtual_register += increment;
    instruction_data.instruction_functions.back().instruction_triplets.push_back(triplet);
}

void InstructionGenerator::visit_function(uptr<function>& func)
{
    if (func->is_extern) return;
    current_table = global_table->add_table().get();
   
    int parameter_position = 0;
    for (auto& param : func->params)
    {
        current_table->add_variable_symbol(param->var_identifier->name,
                param->var_type, parameter_position, true);
        parameter_position++;
    }

    instruction_data.instruction_functions.push_back({func->identifier->name});
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
    emit_instruction(triplet);
}
void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) 
{
    
    VariableSymbol sym = current_table->find_variable_symbol(identifier_literal->name);
    if (sym.is_parameter)
    {
        RegisterSize reg_size = get_register_size(sym.type_info.byte_size);
        InstructionTriplet triplet = 
            InstructionTriplet(Instruction::LOAD_PARAM, 
                    current_virtual_register,
                    {Item{ItemType::PARAMETER_INDEX, sym.vr}},
                    reg_size);

        effective_register = current_virtual_register;
        emit_instruction(triplet);
    }
    else
    {
        effective_register = sym.vr;
    }

}
void InstructionGenerator::visit_binop(uptr<binop_expr>& binop)  
{ 
    //TODO CHECK OP ASSOCIATIVITY
    //TODO "SMARTER" MORE EFFECTIVE SYSTEM RATHER THAN ALWAYS MOV THE LEFT SIDE SO SHIT DONT BREAK
    visit_expression(binop->left);
    virtual_register left = effective_register;
    visit_expression(binop->right);
    virtual_register right = effective_register;
    
    InstructionTriplet left_side_triplet = InstructionTriplet(
        Instruction::STORE,
        current_virtual_register,
        {Item{ItemType::VIRTUAL_REGISTER, left}},
        RegisterSize::BIT64
    );
    effective_register = current_virtual_register;
    emit_instruction(left_side_triplet);

    Instruction instruc;
    switch (binop->op_token)
    {
        case TokenType::PLUS:
            instruc = Instruction::ADD;
            break;
        case TokenType::MINUS:
            instruc = Instruction::SUB;
            break;
        case TokenType::MULTIPLY:
            instruc = Instruction::MUL;
            break;
        case TokenType::DIVIDE:
            instruc = Instruction::DIV;
            break;
        case TokenType::EQU:
            instruc = Instruction::STORE;
            break;
        default:
            //TODO IMPLEMENT MORE
            instruc = Instruction::ADD;
            break;
    }
    /*
    InstructionTriplet triplet = 
        InstructionTriplet(instruc, 
                left,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                RegisterSize::BIT64);
    effective_register = left;
    */
    InstructionTriplet triplet = 
        InstructionTriplet(instruc, 
                effective_register,
                {Item{ItemType::VIRTUAL_REGISTER, right}},
                RegisterSize::BIT64);
    emit_instruction(triplet, 0);

}
void InstructionGenerator::visit_unary(uptr<unary_expr>& unary)  
{
    visit_expression(unary->expr);
}
void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call) 
{
    FunctionSymbol s = current_table->find_function_symbol(function_call->identifier->name);
    uint32_t label = s.id;
    std::vector<Item> parameter_virtual_registers = 
    {Item{ItemType::FUNCTIONTABLE_INDEX, label}};

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
        // save previous current_virtual_register 
        parameter_virtual_registers.push_back(
                Item{ItemType::VIRTUAL_REGISTER, effective_register}); 
    }
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
                RegisterSize::BIT64 
            );
            emit_instruction(align_triplet, 0);
            did_allignment = true;
        }
    }
    // push in reverse order
    for (int i = push_param_triplets.size()-1; i >= 0; i--) emit_instruction(push_param_triplets[i]);

    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::CALL, 
                current_virtual_register,
                parameter_virtual_registers,
                RegisterSize::BIT64);
    effective_register = current_virtual_register;
    emit_instruction(triplet);
    if (did_allignment)
    {
        InstructionTriplet align_triplet(
            Instruction::ALIGN,
            -1,
            {Item{ItemType::IMMEDIATE_VALUE, 8 + 8*pushed_param_count}},
            RegisterSize::BIT64 
        );
        emit_instruction(align_triplet, 0);
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
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::RETURN, 
                 effective_register,
                {},
                RegisterSize::BIT64);
    effective_register = current_virtual_register;
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
    visit_expression(variable_definition->definition);
    visit_variable_declaration(variable_definition->declaration);

    VariableSymbol sym = current_table->find_variable_symbol(
            variable_definition->declaration->var_identifier->name);
    
    RegisterSize reg_size = get_register_size(sym.type_info.byte_size);
    InstructionTriplet triplet = 
        InstructionTriplet(Instruction::STORE, 
                           current_virtual_register,
                           {Item{ItemType::VIRTUAL_REGISTER, effective_register}},
                           reg_size);
    
    std::println("DECLARING SYMBOL {} {}", variable_definition->declaration->var_identifier->name, current_virtual_register);
    emit_instruction(triplet);
    /*InstructionTriplet triplet = 
        InstructionTriplet(Instruction::STORE, 
                 definition,
                {Item{ItemType::VIRTUAL_REGISTER, sym.vr}
                ,Item{ItemType::VIRTUAL_REGISTER, definition}},
                reg_size);

    emit_instruction(triplet, 0);*/
}
void InstructionGenerator::visit_conditional(uptr<conditional_statement>& conditional) 
{
    visit_expression(conditional->condition);
    visit_statement(conditional->then_stat);
    visit_statement(conditional->else_stat);

}
void InstructionGenerator::visit_while(uptr<while_statement>& while_s) 
{
    visit_expression(while_s->condition);
    visit_statement(while_s->loop);
}
void InstructionGenerator::visit_expression_s(uptr<expression_statement>& expr) 
{
    visit_expression(expr->expr);
}
void InstructionGenerator::visit_noop(uptr<noop_statement>& noop) 
{
} 




}

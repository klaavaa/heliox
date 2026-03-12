#include "heliox_instruction_gen.hpp"
namespace hx
{

void InstructionGenerator::visit_program(uptr<Program>& prog)
{
    global_table = std::make_unique<SymbolTable>(); 
    
    
    for (auto& func : prog->functions)
    {
        global_table->add_symbol(
                func->identifier->name, SymbolType::FUNCTION, func->type);
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
    for (auto& func : instruc_data.instruction_functions)
    {
        for (auto& triplet : func.instruction_triplets)
        {
            switch (triplet.instruc)
            {
            case instruction::DIV:
               // TODO: div idiv, unsigned vs signed division
                // TODO: xor rdx, rdx
                // reserves registers RAX, RDX, output in RAX
                break;
            case instruction::CALL:
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
               if(i.it == item_type::VIRTUAL_REGISTER)
               {
                    used_registers.push_back(i.value);
               }
            }
            
            if (triplet.dst >= instruc_data.live_ranges.size())
                instruc_data.live_ranges.push_back(live_range{triplet.dst, instruc_count, 0});

            for (auto& vreg : used_registers)
            {
                if (vreg < instruc_data.live_ranges.size() )
                {
                    instruc_data.live_ranges[vreg].last_use = instruc_count; 
                }
            }
            instruc_count++;

        }
    }
    

    for (size_t i = 0; i < instruc_data.live_ranges.size(); i++)
    {
        std::println("r{} [{} -> {}]", i, instruc_data.live_ranges[i].first_use,
                instruc_data.live_ranges[i].last_use);
    }
}
    
void InstructionGenerator::emit_instruction(instruction_triplet triplet, uint32_t increment)
{
    triplet.instruc_count = instruction_count;
    instruction_count ++;
    print_instruction(triplet);
    current_virtual_register += increment;
    instruc_data.instruction_functions.back().instruction_triplets.push_back(triplet);
}

void InstructionGenerator::visit_function(uptr<function>& func)
{
    if (func->is_extern) return;
    current_table = global_table->add_table(true).get();
   
    for (auto& param : func->params)
    {
        current_table->add_symbol(param->var_identifier->name, SymbolType::VARIABLE,
                param->var_type, true);
    }

    instruc_data.instruction_functions.push_back({func->identifier->name});
    for (auto& stat : func->statements)
    {
        visit_statement(stat);
    }
}

void InstructionGenerator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    instruction_triplet triplet = 
        instruction_triplet(instruction::LOAD_INT, 
                current_virtual_register,
                {item{item_type::IMMEDIATE_VALUE, std::stol(int_literal->value)}},
                register_size::BIT64);
    effective_register = current_virtual_register;
    emit_instruction(triplet);
}
void InstructionGenerator::visit_string_literal(uptr<string_literal_expr>& string_literal)  
{
    uint32_t label = global_table->add_string(string_literal->value);
    instruction_triplet triplet = 
        instruction_triplet(instruction::LOAD_STRING, 
                current_virtual_register,
                {item{item_type::LOOKUPTABLE_INDEX, label}},
                register_size::BIT64);
    effective_register = current_virtual_register;
    emit_instruction(triplet);
}
void InstructionGenerator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) 
{
    // TODO SYMBOL TABLE
    
    Symbol sym = current_table->find_symbol(identifier_literal->name, SymbolType::VARIABLE);
    register_size reg_size = get_register_size(sym.type_info.byte_size);
    instruction_triplet triplet = 
        instruction_triplet(instruction::LOAD_VAR, 
                current_virtual_register,
                {item{item_type::RELATIVE_ADDRESS, sym.stack_position}},
                reg_size);
    if (sym.is_param)
    {
        triplet = 
        instruction_triplet(instruction::LOAD_VAR, 
                current_virtual_register,
                {item{item_type::PARAMETER_INDEX, sym.stack_position}},
                reg_size);
    }
    effective_register = current_virtual_register;
    emit_instruction(triplet);

}
void InstructionGenerator::visit_binop(uptr<binop_expr>& binop)  
{ 
    //TODO CHECK OP ASSOCIATIVITY
    visit_expression(binop->left);
    virtual_register left = effective_register;
    visit_expression(binop->right);
    virtual_register right = effective_register;
    
    instruction instruc;
    switch (binop->op_token)
    {
        case TokenType::PLUS:
            instruc = instruction::ADD;
            break;
        case TokenType::MINUS:
            instruc = instruction::SUB;
            break;
        case TokenType::MULTIPLY:
            instruc = instruction::MUL;
            break;
        case TokenType::DIVIDE:
            instruc = instruction::DIV;
            break;
        default:
            //TODO IMPLEMENT MORE
            instruc = instruction::ADD;
            break;
    }
    instruction_triplet triplet = 
        instruction_triplet(instruc, 
                left,
                {item{item_type::VIRTUAL_REGISTER, right}},
                register_size::BIT64);
    effective_register = left;
    emit_instruction(triplet, 0);

}
void InstructionGenerator::visit_unary(uptr<unary_expr>& unary)  
{
    visit_expression(unary->expr);
}
void InstructionGenerator::visit_function_call(uptr<function_call_expr>& function_call) 
{
    Symbol s = current_table->find_symbol(function_call->identifier->name, SymbolType::FUNCTION);
    uint32_t label = global_table->get_function_id(function_call->identifier->name);
    std::vector<item> parameter_virtual_registers = 
    {item{item_type::LOOKUPTABLE_INDEX, label}};
    for (auto& param : function_call->parameters)
    {
        visit_expression(param);
        // save previous current_virtual_register 
        parameter_virtual_registers.push_back(
                item{item_type::VIRTUAL_REGISTER, effective_register}); 
    }
    
    instruction_triplet triplet = 
        instruction_triplet(instruction::CALL, 
                current_virtual_register,
                parameter_virtual_registers,
                register_size::BIT64);
    effective_register = current_virtual_register;
    emit_instruction(triplet);
}

void InstructionGenerator::visit_compound(uptr<compound_statement>& compound) 
{
    current_table = current_table->add_table(false).get();
    for (auto& stat : compound->statements)
    {
        visit_statement(stat);
    }
    current_table = current_table->get_parent();
}
void InstructionGenerator::visit_return(uptr<return_statement>& return_s) 
{
    visit_expression(return_s->return_expression);
    instruction_triplet triplet = 
        instruction_triplet(instruction::RETURN, 
                 effective_register,
                {},
                register_size::BIT64);
    effective_register = current_virtual_register;
    emit_instruction(triplet, 0);

}
void InstructionGenerator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) 
{
   
    current_table->add_symbol(
            variable_declaration->var_identifier->name,
            SymbolType::VARIABLE,
            variable_declaration->var_type);
}
void InstructionGenerator::visit_variable_definition(uptr<variable_definition_statement>& variable_definition) 
{
    
    visit_variable_declaration(variable_definition->declaration);
    visit_expression(variable_definition->definition);
    virtual_register definition = effective_register;

    Symbol sym = current_table->find_symbol(
            variable_definition->declaration->var_identifier->name, SymbolType::VARIABLE);
    
    register_size reg_size = get_register_size(sym.type_info.byte_size);
    instruction_triplet triplet = 
        instruction_triplet(instruction::STORE, 
                 definition,
                {item{item_type::RELATIVE_ADDRESS, sym.stack_position}
                ,item{item_type::VIRTUAL_REGISTER, definition}},
                reg_size);
    emit_instruction(triplet, 0);

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

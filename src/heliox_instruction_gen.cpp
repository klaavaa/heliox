#include "heliox_instruction_gen.hpp"

namespace hx
{

void instruction_generator::visit_program(uptr<program>& prog)
{
    global_table = std::make_unique<symbol_table>(); 
    
    
    for (auto& func : prog->functions)
    {
        std::println("FUNC NAME {}_", func->identifier->name);
        global_table->add_symbol(
                func->identifier->name, symbol_type::FUNCTION, func->type);
     
    }
    for (auto& func : prog->functions)
    {
       visit_function(func); 
    }

}
    
void instruction_generator::emit_instruction(instruction_triplet triplet, uint32_t increment)
{
    print_instruction(triplet);
    current_virtual_register += increment;
    instruction_functions.back().triplet_queue.push(triplet);
}

void instruction_generator::visit_function(uptr<function>& func)
{
    current_table = global_table->add_table(true).get();
   
    for (auto& param : func->params)
    {
        current_table->add_symbol(param->var_identifier->name, symbol_type::VARIABLE,
                param->var_type);
    }

    instruction_functions.push({func->identifier->name});
    for (auto& stat : func->statements)
    {
        visit_statement(stat);
    }
}

void instruction_generator::visit_int_literal(uptr<int_literal_expr>& int_literal) 
{
    instruction_triplet triplet = 
        instruction_triplet(instruction::LOAD_INT, 
                current_virtual_register,
                {item{item_type::IMMEDIATE_VALUE, std::stol(int_literal->value)}});
    emit_instruction(triplet);
}
void instruction_generator::visit_string_literal(uptr<string_literal_expr>& string_literal)  
{
    instruction_triplet triplet = 
        instruction_triplet(instruction::LOAD_STRING, 
                current_virtual_register,
                {item{item_type::LOOKUPTABLE_INDEX, string_literal->label}});
    emit_instruction(triplet);
}
void instruction_generator::visit_identifier_literal(uptr<identifier_literal_expr>& identifier_literal) 
{
    // TODO SYMBOL TABLE
    symbol sym = current_table->find_symbol(identifier_literal->name, symbol_type::VARIABLE);
    instruction_triplet triplet = 
        instruction_triplet(instruction::LOAD_VAR, 
                current_virtual_register,
                {item{item_type::RELATIVE_ADDRESS, sym.stack_position}});
    emit_instruction(triplet);

}
void instruction_generator::visit_binop(uptr<binop_expr>& binop)  
{ 
    //TODO CHECK OP ASSOCIATIVITY
    visit_expression(binop->left);
    virtual_register left = current_virtual_register - 1;
    visit_expression(binop->right);
    virtual_register right = current_virtual_register - 1;
    
    instruction instruc;
    switch (binop->op_token)
    {
        case tk_type::PLUS:
            instruc = instruction::ADD;
            break;
        case tk_type::MINUS:
            instruc = instruction::SUB;
            break;
        case tk_type::MULTIPLY:
            instruc = instruction::MUL;
            break;
        case tk_type::DIVIDE:
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
                {item{item_type::VIRTUAL_REGISTER, right}});
    emit_instruction(triplet, 0);

}
void instruction_generator::visit_unary(uptr<unary_expr>& unary)  
{
    visit_expression(unary->expr);
}
void instruction_generator::visit_function_call(uptr<function_call_expr>& function_call) 
{
    symbol s = current_table->find_symbol(function_call->identifier->name, symbol_type::FUNCTION);
    std::vector<item> parameter_virtual_registers = 
    {item{item_type::LOOKUPTABLE_INDEX, function_call->label}};
    for (auto& param : function_call->parameters)
    {
        visit_expression(param);
        // save previous current_virtual_register 
        parameter_virtual_registers.push_back(
                item{item_type::VIRTUAL_REGISTER, current_virtual_register - 1}); 
    }
    
    instruction_triplet triplet = 
        instruction_triplet(instruction::CALL, 
                current_virtual_register,
                parameter_virtual_registers);
    emit_instruction(triplet);
}

void instruction_generator::visit_compound(uptr<compound_statement>& compound) 
{
    current_table = current_table->add_table(false).get();
    for (auto& stat : compound->statements)
    {
        visit_statement(stat);
    }
    current_table = current_table->get_parent();
}
void instruction_generator::visit_return(uptr<return_statement>& return_s) 
{
    visit_expression(return_s->return_expression);
    instruction_triplet triplet = 
        instruction_triplet(instruction::RETURN, 
                 current_virtual_register,
                {item{item_type::VIRTUAL_REGISTER, current_virtual_register - 1}});
    emit_instruction(triplet, 0);

}
void instruction_generator::visit_variable_declaration(uptr<variable_declaration_statement>& variable_declaration) 
{
   
    current_table->add_symbol(
            variable_declaration->var_identifier->name,
            symbol_type::VARIABLE,
            variable_declaration->var_type);
}
void instruction_generator::visit_variable_definition(uptr<variable_definition_statement>& variable_definition) 
{
    
    visit_variable_declaration(variable_definition->declaration);
    visit_expression(variable_definition->definition);
    virtual_register definition = current_virtual_register - 1;

    symbol sym = current_table->find_symbol(
            variable_definition->declaration->var_identifier->name, symbol_type::VARIABLE);
    
    instruction_triplet triplet = 
        instruction_triplet(instruction::LET, 
                 definition,
                {item{item_type::RELATIVE_ADDRESS, sym.stack_position}
                ,item{item_type::VIRTUAL_REGISTER, definition}});
    emit_instruction(triplet, 0);

}
void instruction_generator::visit_conditional(uptr<conditional_statement>& conditional) 
{
    visit_expression(conditional->condition);
    visit_statement(conditional->then_stat);
    visit_statement(conditional->else_stat);

}
void instruction_generator::visit_while(uptr<while_statement>& while_s) 
{
    visit_expression(while_s->condition);
    visit_statement(while_s->loop);
}
void instruction_generator::visit_expression_s(uptr<expression_statement>& expr) 
{
    visit_expression(expr->expr);
}
void instruction_generator::visit_noop(uptr<noop_statement>& noop) 
{
} 




}

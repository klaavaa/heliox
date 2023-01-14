#pragma once

#include <string>
#include <unordered_map>
#include "heliox_parser.hpp"
#include "heliox_evaluator.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_tools.hpp"

class hx_assembly
{
public:

	std::string generate_asm(hx_sptr<hx_program> program)
	{

	 	global_table = generate_symbol_table(program);
		current_scope_table = global_table;
		global_table->print();
		std::string entry;
		entry =
			"section .text\n"
			"global _start\n"
			"_start:\n"
			"push rbp\n"
			"mov rbp, rsp\n"
			"call main\n"
			"leave\n"
			"mov rax, 60\n"
			"xor rdi, rdi\n"
			"syscall\n";


		for (uint32_t i = 0; i < program->functions.size(); i++)
		{
			layer_depth = 0;
			current_scope_table = global_table->get_symbol_table(program->functions[i]->name);
			std::string function_assembly = generate_function_asm(program->functions[i]);
			current_scope_table = current_scope_table->get_parent();
			entry += function_assembly;
		}

		return entry;
	}

private:

	std::string generate_function_asm(hx_sptr<hx_function> function)
	{
		std::string base =
			string_format(
				"global %s\n"
				"%s:\n"
				"push rbp\n"
				"mov rbp, rsp\n", function->name.c_str(), function->name.c_str()
			);

		base += generate_statement_asm(function->statement);

		base +=
			string_format(
				"leave\n"
				"ret\n"
			);
				
		//delete function;

		return base;

	}

	std::string generate_statement_asm(hx_sptr<hx_statement> statement)
	{
		switch (statement->s_type)
		{
		case statement_type::DEFINITION: return generate_definition_asm(std::dynamic_pointer_cast<hx_definition_statement>(statement));
		case statement_type::COMPOUND:
		{
		
			current_scope_table = current_scope_table->get_table_based_on_index(layer_depth);
			layer_depth++;
			std::string base = generate_compound_asm(std::dynamic_pointer_cast<hx_compound_statement>(statement));
			if (layer_depth -1 > 0)
				current_scope_table = current_scope_table->get_parent();
			
			return base;
		}
		case statement_type::RETURN: return generate_return_asm(std::dynamic_pointer_cast<hx_return_statement>(statement));
		case statement_type::NOOP:  return "";
		}
	}

	std::string generate_compound_asm(hx_sptr<hx_compound_statement> statement)
	{

		std::string base;
		for (uint32_t i = 0; i < statement->statements.size(); i++)
		{
			base += generate_statement_asm(statement->statements[i]);
		}

		return base;
	}

	std::string generate_return_asm(hx_sptr<hx_return_statement> statement)
	{
		std::string base;

		if (statement->expression->e_type == expression_type::IDENTIFIER_LITERAL)
		{
	
			hx_symbol symbol = current_scope_table->find_symbol(std::dynamic_pointer_cast<hx_identifier_literal_expression>(statement->expression)->name);
			
			base = string_format(
				"mov rax, dword[rbp-%d]\n",
				symbol.stack_position
			);
		}
		else
		{
			int64_t value = evaluate_expression(statement->expression);

			base = string_format(
				"mov rax, %d\n"
				, value
			);
		}
		return base;
	}

	std::string generate_definition_asm(hx_sptr<hx_definition_statement> statement)
	{
		
		
		int64_t value = evaluate_expression(statement->expression);
	
		hx_symbol symbol = current_scope_table->get_symbol(statement->type_decl->name);

		push_to_stack(hx_get_size(symbol.data_type));
		
		std::string base;

		base =
			string_format(
				"sub rsp, %d\n"
				"mov dword[rbp-%d], %d\n", hx_get_size(symbol.data_type), symbol.stack_position, value);

		
		return base;		
	}

	std::string generate_expression_asm(hx_sptr<hx_expression> expression)
	{

	}

private:

	void push_to_stack(uint32_t size)
	{
		stack_top_ptr += size;
	}

private:
	hx_symbol_table* global_table;
	hx_symbol_table* current_scope_table;


	std::string text_section;
	std::string bss_section;
	std::string data_section;

	uint32_t layer_depth = 0;

	int32_t stack_base_ptr = 0;
	int32_t stack_top_ptr = 0;



};




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
			"mov rdi, rax\n"
			"mov rax, 60\n"
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
		base += generate_expression_asm(statement->expression);
		return base;
	}

	std::string generate_definition_asm(hx_sptr<hx_definition_statement> statement)
	{
		
		std::string base;
	
		base += generate_expression_asm(statement->expression);

		hx_symbol symbol = current_scope_table->get_symbol(statement->type_decl->name);

		
		

		base +=
			string_format(
				"sub rsp, %d\n"
				"mov qword[rbp-%d], rax\n", hx_get_size(symbol.data_type), symbol.stack_position);

		
		return base;		
	}

	std::string generate_expression_asm(hx_sptr<hx_expression> expression)
	{


		std::string base;

		if (expression->e_type == expression_type::IDENTIFIER_LITERAL)
			base += "mov rax, " + generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression)) + "\n";
		else
		{
			bool can_be_evaluated_fully = true;
			int64_t value = evaluate_expression(expression, can_be_evaluated_fully);
			
			if (can_be_evaluated_fully)
			{
				base = string_format(
					"mov rax, %d\n"
					, value
				);
			}
			else
			{
					base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression));
				
			}
		}
		return base;
	}

	std::string generate_identifier_literal_asm(hx_sptr<hx_identifier_literal_expression> expression)
	{
		std::string base;
		hx_symbol symbol = current_scope_table->find_symbol(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression)->name);
		
		base = string_format(
			"qword[rbp-%d]",
			symbol.stack_position
		);

		return base;

	}

	std::string generate_binop_asm(hx_sptr<hx_binop_expression> expression)
	{
		std::string base;

		expression_type left_type = expression->left->e_type;
		expression_type right_type = expression->right->e_type;

		if (left_type == expression_type::BINOP)
		{
			base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression->left));
			base += "mov r8, rax\n";
		}

		if (right_type == expression_type::BINOP)
		{
			base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression->right));
			base += "mov rcx, rax\n";
		}

		if (left_type == expression_type::BINOP)
		{
			base += "mov rax, r8\n";
		}


		if (left_type == expression_type::IDENTIFIER_LITERAL)
		{
			base += "mov rax, " + generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->left)) + "\n";
		}

		else if (left_type == expression_type::INT_LITERAL)
		{
			int64_t value = evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(expression->left));

			base += string_format(
					"mov rax, %d\n", value);


		}

		if (right_type == expression_type::BINOP)
		{
			if (expression->op == tk_type_to_str::get_str(tk_type::TK_PLUS))
			{
				base += string_format(
					"add rax, rcx\n");
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_MINUS))
			{
				base += string_format(
					"sub rax, rcx\n");
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_MULTIPLY))
			{
				base += string_format(
					"imul rax, rcx\n");
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_DIVIDE))
			{

				base += string_format(
					"xor rdx, rdx\n"
					"idiv rcx\n");
			}
		}

		else if (right_type == expression_type::IDENTIFIER_LITERAL)
		{
			if (expression->op == tk_type_to_str::get_str(tk_type::TK_PLUS))
			{
				base += string_format(
					"add rax, %s\n", generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_MINUS))
			{
				base += string_format(
					"sub rax, %s\n", generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_MULTIPLY))
			{
				base += string_format(
					"imul rax, %s\n", generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_DIVIDE))
			{

				base += string_format(
					"xor rdx, rdx\n"
					"mov r8, %s\n"
					"idiv r8\n", generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
			}
		}
		
		else if (right_type == expression_type::INT_LITERAL)
		{
			int64_t value = evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(expression->right));
			if (expression->op == tk_type_to_str::get_str(tk_type::TK_PLUS))
			{
				base += string_format(
					"add rax, %d\n", value);
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_MINUS))
			{
				base += string_format(
					"sub rax, %d\n", value);
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_MULTIPLY))
			{
				base += string_format(
					"imul rax, %d\n", value);
			}
			else if (expression->op == tk_type_to_str::get_str(tk_type::TK_DIVIDE))
			{
				base += string_format(
					"xor rdx, rdx\n"
					"mov r8, %d\n"
					"idiv r8\n", value);
			}
		}

		return base;
	}

private:
	hx_symbol_table* global_table;
	hx_symbol_table* current_scope_table;


	std::string text_section;
	std::string bss_section;
	std::string data_section;

	uint32_t layer_depth = 0;



};




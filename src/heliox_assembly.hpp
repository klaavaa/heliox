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
			"section .bss\n"
			"\tbuf resb 80\n"
			"section .text\n"
			"global _start\n"
			"_start:\n"
			"\tpush rbp\n"
			"\tmov rbp, rsp\n"
			"\tcall main\n"
			"\tleave\n"

			"\tmov r12, rax\n"
			"\tmov rdi, rax\n"
			"\tmov rax, buf\n"
			"\tcall uitoa\n"
			"\tmov rdx, rax\n"
			"\tmov rsi, buf\n"
			"\tmov rdi, 1\n"
			"\tmov rax, 1\n"
			"\tsyscall\n"

			"\tmov rax, 60\n"
			"\tsyscall\n";


		for (uint32_t i = 0; i < program->functions.size(); i++)
		{
			layer_depth = 0;
			current_scope_table = global_table->get_symbol_table(program->functions[i]->name);
			std::string function_assembly = generate_function_asm(program->functions[i]);
			current_scope_table = current_scope_table->get_parent();
			entry += function_assembly;
		}

		entry +=
			"uitoa:\n"
			"mov 	rsi, rax\n"
			"mov 	rax, rdi\n"

			"cmp 	rax, 0\n"
			"jnz 	uitoa_convert_regular\n"
			"mov 	byte[rsi], 48\n"
			"inc    	esi\n"
			"mov 	byte[rsi], 0\n"
			"mov 	rax, 1\n"
			"jmp 	uitoa_end\n"

			"uitoa_convert_regular:\n"
			"mov 	r10, 10\n"


			"xor rcx, rcx\n"
			"uitoa_loop :\n"
			"xor rdx, rdx\n"
			"div 	r10\n"
			"inc	ecx\n"
			"cmp 	rax, 0\n"
			"jnz 	uitoa_loop\n"
			"inc 	ecx\n"
			"mov 	r8, rcx\n"

			"add 	rsi, rcx\n"
			"mov 	byte[rsi], 0\n"
			"mov 	rax, rdi\n"
			"dec 	ecx\n"

			"uitoa_convert :\n"
			"xor rdx, rdx\n"
			"dec 	rsi\n"
			"div 	r10\n"
			"add 	rdx, 48\n"
			"mov 	byte[rsi], dl\n"
			"loopnz  uitoa_convert\n"

			"mov 	rax, r8\n"
			"uitoa_end :\n"
			"ret\n";


		return entry;
	}

private:

	std::string generate_function_asm(hx_sptr<hx_function> function)
	{
		std::string base =
			string_format(
				"global %s\n"
				"%s:\n"
				"\tpush rbp\n"
				"\tmov rbp, rsp\n", function->name.c_str(), function->name.c_str()
			);

		base += generate_statement_asm(function->statement);

		base += "\tleave\n";
		base += "\tret\n";
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
		case statement_type::CONDITIONAL: 
			return generate_conditional_asm(std::dynamic_pointer_cast<hx_conditional_statement>(statement));
		case statement_type::RETURN: 
			return generate_return_asm(std::dynamic_pointer_cast<hx_return_statement>(statement));
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

	std::string generate_conditional_asm(hx_sptr<hx_conditional_statement> statement)
	{
		std::string base;
		base += generate_expression_asm(statement->expression);

		base += "\ttest rax, rax\n";
		base += string_format("\tjz IFEND%d\n", label_counter);

		base += generate_statement_asm(statement->statement);

		base += string_format("IFEND%d:\n", label_counter);

		label_counter++;
		return base;
	}

	std::string generate_return_asm(hx_sptr<hx_return_statement> statement)
	{
		std::string base;
		base += generate_expression_asm(statement->expression);
		base +=
			string_format(
				"\tleave\n"
				"\tret\n"
			);
		return base;
	}

	std::string generate_definition_asm(hx_sptr<hx_definition_statement> statement)
	{
		
		std::string base;
	
		base += generate_expression_asm(statement->expression);

		hx_symbol symbol = current_scope_table->get_symbol(statement->type_decl->name);

	

		base +=
			string_format(
				"\tsub rsp, %d\n"
				"\tmov qword[rbp-%d], rax	; variable: %s\n", hx_get_size(symbol.data_type), symbol.stack_position, statement->type_decl->name.c_str());

		
		return base;		
	}

	std::string generate_expression_asm(hx_sptr<hx_expression> expression)
	{


		std::string base;

		switch (expression->e_type)
		{

		case expression_type::IDENTIFIER_LITERAL:
		{
			base += "\tmov rax, " + generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression)) + "\n";
			break;
		}
		case expression_type::FUNCTION_CALL:
		{
			hx_sptr<hx_function_call_expression> f_expr = std::dynamic_pointer_cast<hx_function_call_expression>(expression);
			base += generate_function_call_asm(f_expr);
			break;
		}
		case expression_type::INT_LITERAL:
		{
			std::optional<int64_t> opt = evaluate_expression(expression);
			int64_t value = opt.value();

			base = string_format(
				"\tmov rax, qword %lld\n"
				, value
			);

			break;
		}
		case expression_type::BINOP:
		{
			std::optional<int64_t> opt = evaluate_expression(expression);

			if (opt.has_value())
			{
				int64_t value = opt.value();
				base = string_format(
					"\tmov rax, qword %lld\n"
					, value
				);
			}
			else
			{
				//std::dynamic_pointer_cast<hx_binop_expression>(expression)->print();
				//printf("\n");
				base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression));

			}
			break;
		}
		}
		return base;
	}

	std::string generate_identifier_literal_asm(hx_sptr<hx_identifier_literal_expression> expression)
	{
		std::string base;

		hx_symbol symbol = current_scope_table->find_symbol(
			std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression)->name,
			hx_symbol_type::VAR, expression->line_number);
		
		
		base = string_format(
			"qword[rbp-%d]",
			symbol.stack_position
		);
		
	

		return base;

	}

	std::string generate_function_call_asm(hx_sptr<hx_function_call_expression> fn_call)
	{

		std::string base;
		
		for (int32_t i = fn_call->arguments.size()-1; i >= 0; i--)
		{
			base += generate_expression_asm(fn_call->arguments[i]);
			base += "\tpush rax\n";
		}
		
		base += "\tcall " + fn_call->identifier->name + "\n";
		base += string_format("\tadd rsp, %d\n", fn_call->arguments.size() * 8);
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
			base += "\tmov r8, rax\n";
		}

		if (right_type == expression_type::BINOP)
		{
			base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression->right));
			base += "\tmov rcx, rax\n";
		}

		switch (left_type)
		{
		case expression_type::BINOP:
		{
			base += "\tmov rax, r8\n";
			break;
		}
		case expression_type::IDENTIFIER_LITERAL:
		{
			base += "\tmov rax, " + generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->left)) + "\n";
			break;
		}
		case expression_type::INT_LITERAL:
		{
			std::optional<int64_t> opt = evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(expression->left));
			int64_t value = opt.value();
			base += string_format(
				"\tmov rax, qword %lld\n", value);
			break;
		}
		case expression_type::FUNCTION_CALL:
			hx_sptr<hx_function_call_expression> f_expr = std::dynamic_pointer_cast<hx_function_call_expression>(expression->left);
			base += generate_function_call_asm(f_expr);
			break;
		}

		tk_type op = tk_type_to_str::get_tk(expression->op.c_str());


		switch (right_type)
		{
		case expression_type::BINOP:
		{
			switch (op)
			{
			case tk_type::TK_PLUS:
				base += string_format(
					"\tadd rax, rcx\n");
				break;
			case tk_type::TK_MINUS:
				base += string_format(
					"\tsub rax, rcx\n");
				break;
			case tk_type::TK_MULTIPLY:
				base += string_format(
					"\tsub rax, rcx\n");
				break;
			case tk_type::TK_DIVIDE:
				base += string_format(
					"\txor rdx, rdx\n"
					"\tidiv rcx\n");
				break;
			case tk_type::TK_GT:
				base += string_format(
					"\tcmp rax, rcx\n"
					"\tmov rax, 1\n"
					"\tjg GREATER%d\n"
					"\txor rax, rax\n"
					"GREATER%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_GTE:
				base += string_format(
					"\tcmp rax, rcx\n"
					"\tmov rax, 1\n"
					"\tjge GREATEREQU%d\n"
					"\txor rax, rax\n"
					"GREATEREQU%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LT:
				base += string_format(
					"\tcmp rax, rcx\n"
					"\tmov rax, 1\n"
					"\tjl LESS%d\n"
					"\txor rax, rax\n"
					"LESS%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LTE:
				base += string_format(
					"\tcmp rax, rcx\n"
					"\tmov rax, 1\n"
					"\tjle LESSEQU%d\n"
					"\txor rax, rax\n"
					"LESSEQU%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;

			case tk_type::TK_DOUBLE_EQU:

				base += string_format(
					"\tcmp rax, rcx\n"
					"\tmov rax, 1\n"
					"\tjz EQUAL%d\n"
					"\txor rax, rax\n"
					"EQUAL%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_NEQU:

				base += string_format(
					"\tcmp rax, rcx\n"
					"\tmov rax, 1\n"
					"\tjnz NOTEQUAL%d\n"
					"\txor rax, rax\n"
					"NOTEQUAL%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LOGICAL_AND:
				base += "\tand rax, rcx\n";
				break;
			case tk_type::TK_LOGICAL_OR:
				base += "\tor rax, rcx\n";
				break;
				
			}
			break;
		}


		case expression_type::IDENTIFIER_LITERAL:
		{
			switch (op)
			{
			case tk_type::TK_PLUS:
				base += string_format(
					"\tadd rax, %s\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
				break;
			case tk_type::TK_MINUS:
				base += string_format(
					"\tsub rax, %s\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
				break;
			case tk_type::TK_MULTIPLY:
				base += string_format(
					"\timul rax, %s\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
				break;
			case tk_type::TK_DIVIDE:
				base += string_format(
					"\txor rdx, rdx\n"
					"\tmov r9, %s\n"
					"\tidiv r9\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
				break;
			case tk_type::TK_GT:
				base += string_format(
					"\tcmp rax, %s\n"
					"\tmov rax, 1\n"
					"\tjg GREATER%d\n"
					"\txor rax, rax\n"
					"GREATER%d:\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str(),
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_GTE:
				base += string_format(
					"\tcmp rax, %s\n"
					"\tmov rax, 1\n"
					"\tjge GREATEREQU%d\n"
					"\txor rax, rax\n"
					"GREATEREQU%d:\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str(),
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LT:
				base += string_format(
					"\tcmp rax, %s\n"
					"\tmov rax, 1\n"
					"\tjl LESS%d\n"
					"\txor rax, rax\n"
					"LESS%d:\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str(),
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LTE:
				base += string_format(
					"\tcmp rax, %s\n"
					"\tmov rax, 1\n"
					"\tjle LESSEQU%d\n"
					"\txor rax, rax\n"
					"LESSEQU%d:\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str(),
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_DOUBLE_EQU:

				base += string_format(
					"\tcmp rax, %s\n"
					"\tmov rax, 1\n"
					"\tjz EQUAL%d\n"
					"\txor rax, rax\n"
					"EQUAL%d:\n", 
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str(),
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_NEQU:

				base += string_format(
					"\tcmp rax, %s\n"
					"\tmov rax, 1\n"
					"\tjnz NOTEQUAL%d\n"
					"\txor rax, rax\n"
					"NOTEQUAL%d:\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str(),
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LOGICAL_AND:
				base += string_format(
					"\tand rax, %s\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
				break;
			case tk_type::TK_LOGICAL_OR:
				base += string_format(
					"\tor rax, %s\n",
					generate_identifier_literal_asm(std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)).c_str());
				break;

			}
			break;
		}
		case expression_type::INT_LITERAL:
		{
			std::optional<int64_t> opt = evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(expression->right));
			int64_t value = opt.value();
			//bool is_64_bytes = value > INT32_MAX; 
			// TODO: check if the number is greater than 32 bytes and only then move it to a register for the operation
			//		 else use op rax, imm32

			switch (op)
			{
			case tk_type::TK_PLUS:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tadd rax, r9\n", value);
				break;
			case tk_type::TK_MINUS:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tsub rax, r9\n", value);
				break;
			case tk_type::TK_MULTIPLY:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\timul rax, r9", value);
				break;
			case tk_type::TK_DIVIDE:
				base += string_format(
					"\txor rdx, rdx\n"
					"\tmov r9, qword %lld\n"
					"\tidiv r9\n", value);
				break;
			case tk_type::TK_GT:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjg GREATER%d\n"
					"\txor rax, rax\n"
					"GREATER%d:\n", value, label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_GTE:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjge GREATEREQU%d\n"
					"\txor rax, rax\n"
					"GREATEREQU%d:\n", value, label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LT:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjl LESS%d\n"
					"\txor rax, rax\n"
					"LESS%d:\n", value, label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LTE:
				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjle LESSEQU%d\n"
					"\txor rax, rax\n"
					"LESSEQU%d:\n", value, label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_DOUBLE_EQU:

				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjz EQUAL%d\n"
					"\txor rax, rax\n"
					"EQUAL%d:\n",
					value, label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_NEQU:

				base += string_format(
					"\tmov r9, qword %lld\n"
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjnz NOTEQUAL%d\n"
					"\txor rax, rax\n"
					"NOTEQUAL%d:\n",
					value, label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LOGICAL_AND:
				base += string_format(
					"\tand rax, qword %lld", value);
				break;
			case tk_type::TK_LOGICAL_OR:
				base += string_format(
					"\tor rax, qword %lld", value);
				break;
			}
			break;
		}
		case expression_type::FUNCTION_CALL:
		{
			base += "\tpush rax\n";

			hx_sptr<hx_function_call_expression> f_expr = std::dynamic_pointer_cast<hx_function_call_expression>(expression->right);
			base += generate_function_call_asm(f_expr);

			base += "\tmov r9, rax\n";
			base += "\tpop rax\n";
			switch (op)
			{
			case tk_type::TK_PLUS:
				base += "\tadd rax, r9\n";
				break;
			case tk_type::TK_MINUS:
				base += "\tsub rax, r9\n";
				break;
			case tk_type::TK_MULTIPLY:
				base += "\timul rax, r9\n";
				break;
			case tk_type::TK_DIVIDE:
				base +=
					"\txor rdx, rdx\n"
					"\tidiv r9\n";
				break;
			case tk_type::TK_GT:
				base += string_format(
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjg GREATER%d\n"
					"\txor rax, rax\n"
					"GREATER%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_GTE:
				base += string_format(
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjge GREATEREQU%d\n"
					"\txor rax, rax\n"
					"GREATEREQU%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LT:
				base += string_format(
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjl LESS%d\n"
					"\txor rax, rax\n"
					"LESS%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LTE:
				base += string_format(
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjle LESSEQU%d\n"
					"\txor rax, rax\n"
					"LESSEQU%d:\n", label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_DOUBLE_EQU:

				base += string_format(
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjz EQUAL%d\n"
					"\txor rax, rax\n"
					"EQUAL%d:\n",
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_NEQU:

				base += string_format(
					"\tcmp rax, r9\n"
					"\tmov rax, 1\n"
					"\tjnz NOTEQUAL%d\n"
					"\txor rax, rax\n"
					"NOTEQUAL%d:\n",
					label_counter, label_counter
				);
				label_counter++;
				break;
			case tk_type::TK_LOGICAL_AND:
				base += "\tand rax, r9\n";
				break;
			case tk_type::TK_LOGICAL_OR:
				base += "\tor rax, r9\n";
				break;
			}

			break;
		}
		}
		return base;
	}

private:
	hx_symbol_table* global_table;
	hx_symbol_table* current_scope_table;


	uint32_t layer_depth = 0;
	uint32_t label_counter = 0;


};




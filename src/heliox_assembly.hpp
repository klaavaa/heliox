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

	std::string generate_asm(hx_sptr<hx_program> program, hx_symbol_table* _global_table)
	{

	 	global_table = _global_table;
		current_scope_table = global_table;
		//global_table->print();
		std::string entry;
		entry =
			"section .data\n"
			"\tfmt: db \"%lld\", 10, 0\n"
			"section .bss\n"
			"\tbuf resb 80\n"
			"extern printf\n"
			"section .text\n"
			"global main\n"
			"main:\n"
			"\tpush rbp\n"
			"\tmov rbp, rsp\n"
			"\tcall _main\n"
			
			"\tleave\n"
			"\tmov rax, 60\n"
			"\tsyscall\n";

		entry +=
			"global _print\n"
			"_print:\n"
			"\tpush rbp\n"
			"\tmov rbp, rsp\n"
		
			"\tmov rsi, qword[rbp--16]\n"
			"\tpush rax\n"
			"\tpush rbx\n"
			"\tmov rdi, fmt\n"
			"\txor rax, rax\n"
			"\tcall printf WRT ..plt\n"
			"\tpop rbx\n"
			"\tpop rax\n"
			"\tleave\n"
			"\tret\n";

		for (const auto& function : program->functions)
		{
			if (function->is_declaration)
				continue;
			
			layer_depth = 0;
			current_scope_table = global_table->get_symbol_table(function->name);
			std::string function_assembly = generate_function_asm(function);
			entry += function_assembly;
		}

		return entry;
	}

private:

	std::string generate_function_asm(hx_sptr<hx_function> function)
	{
		std::string base =
			string_format(
				"global _%s\n"
				"_%s:\n"
				"\tpush rbp\n"
				"\tmov rbp, rsp\n"
				, function->name.c_str(), function->name.c_str()
				
			);

		if (current_scope_table->allocated_memory_stack > 0)
			base += string_format("\tsub rsp, %d\n", current_scope_table->allocated_memory_stack);

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
			if (layer_depth > 0)
				current_scope_table = current_scope_table->get_table_based_on_index(layer_depth -1);
			layer_depth++;
			uint32_t preserve = layer_depth;
			std::string base = generate_compound_asm(std::dynamic_pointer_cast<hx_compound_statement>(statement));
			layer_depth = preserve;
			current_scope_table = current_scope_table->get_parent();
			
			return base;
		}
		case statement_type::CONDITIONAL: 
			return generate_conditional_asm(std::dynamic_pointer_cast<hx_conditional_statement>(statement));
		case statement_type::WHILE:
			return generate_while_asm(std::dynamic_pointer_cast<hx_while_statement>(statement));
		case statement_type::RETURN: 
			return generate_return_asm(std::dynamic_pointer_cast<hx_return_statement>(statement));
		case statement_type::NOOP:  return "";

		case statement_type::EXPRESSION:
			return generate_expression_asm(std::dynamic_pointer_cast<hx_expression_statement>(statement)->expression, "rax");
			break;

		default:
		{
			hx_error error;
			error.line = statement->line_number;
			error.error_type = HX_SYNTAX_ERROR;
			error.info = "Unexpected statement found";
			hx_logger::log_and_exit(error);
            return "";
		}
		}

		
	}

	std::string generate_compound_asm(hx_sptr<hx_compound_statement> statement)
	{
		layer_depth = 1;
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

		bool has_else = statement->else_statement.has_value();

		uint32_t cur_label = label_counter;
		label_counter++;

		base += generate_expression_asm(statement->expression, "rax");

		base += "\ttest rax, rax\n";
		base += string_format("\tjz IFEND%d\n", cur_label);

		base += generate_statement_asm(statement->statement);

		if (has_else)
		{
			base += string_format("\tjmp ELSEEND%d\n", cur_label);
		}
		base += string_format("IFEND%d:\n", cur_label);


		if (has_else)
		{
			hx_sptr<hx_statement> else_stat = statement->else_statement.value();

			base += generate_statement_asm(else_stat);

			base += string_format("ELSEEND%d:\n", cur_label);
		}




		return base;
	}

	std::string generate_while_asm(hx_sptr<hx_while_statement> statement)
	{
		std::string base;

		uint32_t cur_label = label_counter;
		label_counter++;

		base += string_format("WHILESTART%d:\n", cur_label);
		base += generate_expression_asm(statement->expression, "rax");
		base += "\ttest rax, rax\n";
		base += string_format("\tjz WHILEEND%d\n", cur_label);

		base += generate_statement_asm(statement->statement);
		base += string_format("\tjmp WHILESTART%d\n", cur_label);
		base += string_format("WHILEEND%d:\n", cur_label);

	
		return base;
	}

	std::string generate_return_asm(hx_sptr<hx_return_statement> statement)
	{
		std::string base;
		base += generate_expression_asm(statement->expression, "rax");
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
		
		if (statement->is_declaration)
			base += generate_expression_asm(statement->expression, "rax");

		hx_symbol symbol = current_scope_table->get_symbol(statement->type_decl->name);

	

		base +=
			string_format(
				"\tmov qword[rbp-%d], rax	; variable: %s\n", symbol.stack_position, statement->type_decl->name.c_str());

		
		return base;		
	}

	std::string generate_expression_asm(hx_sptr<hx_expression> expression, std::string reg)
	{


		std::string base;
		
		std::optional<int64_t> opt = evaluate_expression(expression);
		if (opt.has_value())
		{
			int64_t value = opt.value();

			base = string_format(
				"\tmov %s, qword %lld\n"
				, reg.c_str(), value
			);
			return base;
		}
		base += "\tmov " + reg + ", ";

		switch (expression->e_type)
		{

		case expression_type::IDENTIFIER_LITERAL:
		{
			auto identifier = std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression);
			base += generate_identifier_literal_asm(identifier) + "\n";
			break;
		}
		case expression_type::FUNCTION_CALL:
		{
			base = generate_function_call_asm(std::dynamic_pointer_cast<hx_function_call_expression>(expression));
			break;
		}

		case expression_type::UNARYOP:
		{
			generate_unary_asm(std::dynamic_pointer_cast<hx_unary_expression>(expression), reg);
			break;
		}

		case expression_type::BINOP:
		{
			/*
			std::dynamic_pointer_cast<hx_binop_expression>(expression)->print();
			printf("\n"); */
			base = generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression));
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
		
		for (int i = (int)fn_call->arguments.size()-1; i >= 0; i--)
		{
			base += generate_expression_asm(fn_call->arguments[i], "rax");
			base += "\tpush rax\n";
		}
		
		base += "\tcall _" + fn_call->identifier->name + "\n";
		base += string_format("\tadd rsp, %d\n", fn_call->arguments.size() * 8);
		return base;
	}


	std::string generate_binop_asm(hx_sptr<hx_binop_expression> expression)
	{
		
		std::string base;

		expression_type left_type = expression->left->e_type;
		expression_type right_type = expression->right->e_type;


		hx_operator op(expression->op);

		uint32_t cur_label = label_counter;
		label_counter++;

		if (!op.left_associative)
		{
			std::swap(expression->left, expression->right);
			std::swap(left_type, right_type);
		}

		if (left_type == expression_type::BINOP)
		{
			base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression->left));
			base += "\tmov r8, rax\n";
		}
		else
			base += generate_expression_asm(expression->left, "rax");

		// if left evaluates to false it doesnt evaluate the right 
		if (op.tk == TK_LOGICAL_AND)
		{
			base += "\ttest rax, rax\n";
			base += string_format("\tjz ANDMID%d\n", cur_label);
		}
		else if (op.tk == TK_LOGICAL_OR)
		{
			base += "\ttest rax, rax\n";
			base += "\tmov rax, 1\n";
			base += string_format("\tjnz OREND%d\n", cur_label);
			base += "\tmov rax, 0\n";

		}

		if (right_type == expression_type::BINOP)
		{
			base += generate_binop_asm(std::dynamic_pointer_cast<hx_binop_expression>(expression->right));
			base += "\tmov r9, rax\n";
		}
		else
			base += generate_expression_asm(expression->right, "rbx");

		if (left_type == expression_type::BINOP)
			base += "\tmov rax, r8\n";
	
		if (right_type == expression_type::BINOP)
			base += "\tmov rbx, r9\n";
		
	
		switch (op.tk)
		{

		case tk_type::TK_PLUS:
			base += "\tadd rax, rbx\n";
			break;

		case tk_type::TK_MINUS:
			base += "\tsub rax, rbx\n";
			break;
		case tk_type::TK_MULTIPLY:
			base += "\timul rax, rbx\n";
			break;
		case tk_type::TK_DIVIDE:
			base += "\txor rdx, rdx\n";
			base += "\tidiv rbx\n";
			break;
		case tk_type::TK_EQU:
			base += "\tmov " + generate_identifier_literal_asm(
				std::dynamic_pointer_cast<hx_identifier_literal_expression>(expression->right)) + ", rax\n";
			break;
		case tk_type::TK_LTE:
			base += string_format(
				"\tcmp rax, rbx\n"
				"\tmov rax, 1\n"
				"\tjle LESSEQU%d\n"
				"\txor rax, rax\n"
				"LESSEQU%d:\n", label_counter, label_counter
			);
			label_counter++;
			break;
		case tk_type::TK_LT:
			base += string_format(
				"\tcmp rax, rbx\n"
				"\tmov rax, 1\n"
				"\tjl LESS%d\n"
				"\txor rax, rax\n"
				"LESS%d:\n", label_counter, label_counter
			);
			label_counter++;
			break;
		case tk_type::TK_GTE:
			base += string_format(
				"\tcmp rax, rbx\n"
				"\tmov rax, 1\n"
				"\tjge GREATEREQU%d\n"
				"\txor rax, rax\n"
				"GREATEREQU%d:\n", label_counter, label_counter
			);
			label_counter++;
			break;
		case tk_type::TK_GT:
			base += string_format(
				"\tcmp rax, rbx\n"
				"\tmov rax, 1\n"
				"\tjg GREATER%d\n"
				"\txor rax, rax\n"
				"GREATER%d:\n", label_counter, label_counter
			);
			label_counter++;
			break;

		case tk_type::TK_DOUBLE_EQU:

			base += string_format(
				"\tcmp rax, rbx\n"
				"\tmov rax, 1\n"
				"\tjz EQUAL%d\n"
				"\txor rax, rax\n"
				"EQUAL%d:\n", label_counter, label_counter
			);
			label_counter++;
			break;
		case tk_type::TK_NEQU:

			base += string_format(
				"\tcmp rax, rbx\n"
				"\tmov rax, 1\n"
				"\tjnz NOTEQUAL%d\n"
				"\txor rax, rax\n"
				"NOTEQUAL%d:\n", label_counter, label_counter
			);
			label_counter++;
			break;
		case tk_type::TK_LOGICAL_AND:			
			base += "\ttest rbx, rbx\n";
			base += "\tmov rax, 1\n";
			base += string_format("\tjnz ANDEND%d\n", cur_label);
			
			base += string_format("ANDMID%d:\n", cur_label);
			base += "\tmov rax, 0\n";
			base += string_format("ANDEND%d:\n", cur_label);
			break;
		case tk_type::TK_LOGICAL_OR:
			
			base += "\ttest rbx, rbx\n";
			base += string_format("\tjz OREND%d\n", cur_label);
			base += "\tmov rax, 1\n";
			base += string_format("OREND%d:\n", cur_label);

			break;

		case tk_type::TK_BITWISE_AND:
			base += "\tand rax, rbx\n";
			break;

		case tk_type::TK_BITWISE_OR:
			base += "\tor rax, rbx\n";
			break;

		case tk_type::TK_BITWISE_XOR:
			base += "\txor rax, rbx\n";
			break;

			

		}
		
		return base;
	}

	std::string generate_unary_asm(hx_sptr<hx_unary_expression> expression, std::string reg)
	{

		std::string base;
		hx_operator op(expression->op);

		generate_expression_asm(expression->expression, reg);
		
		switch (op.tk)
		{
		case TK_MINUS:
			base += "\tneg " + reg + "\n";
			break;
		case TK_PLUS:
			break;
		case TK_NOT:
			base += "\ttest " + reg + ", " + reg + "\n";
			base += "\tmov " + reg + ", 1\n";
			base += string_format("\tjz NOTEND%d\n", label_counter);
			base += "\txor " + reg + ", " + reg + "\n";
			base += string_format("NOTEND%d:\n", label_counter);

			label_counter++;
			break;


		}
	
		return base;
	}

	std::string generate_int_literal_asm(hx_sptr<hx_int_literal_expression> expression)
	{
		return string_format("qword %lld", expression->value);
	}



private:
	hx_symbol_table* global_table;
	hx_symbol_table* current_scope_table;


	uint32_t layer_depth = 0;
	uint32_t label_counter = 0;


};


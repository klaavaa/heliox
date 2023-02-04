#include "heliox_symbol_table.hpp"

bool hx_symbol_table::check_if_exists(const std::string& name, hx_symbol_type symbol_type)
{

	if (symbols.count(name))
	{
		if (symbols[name].type == symbol_type)
			return true;
	}

	return false;
}

bool hx_symbol_table::insert(std::string name, hx_symbol symbol)
{
	if (check_if_exists(name, symbol.type))
		return false;

	symbols[name] = symbol;
	return true;
}

void hx_symbol_table::add_symbol_table(std::string name, hx_symbol_table* symbol_table)
{
	symbol_table->parent = this;
	symbol_table->my_name = name;
	symbol_tables[name] = symbol_table;
}

hx_symbol hx_symbol_table::get_symbol(const std::string& name)
{
	return symbols.at(name);
}

hx_symbol hx_symbol_table::find_symbol(const std::string& name, hx_symbol_type symbol_type, uint32_t line_number)
{
	
	if (check_if_exists(name, symbol_type))
	{
		if (line_number > symbols.at(name).line_number)
			return symbols.at(name);
	}
	
	if (this->parent)
		return this->parent->find_symbol(name, symbol_type, line_number);

	hx_error err;
	err.line = line_number;
	err.ok = false;
	err.file = "";
	err.error_type = HX_SYMBOL_NOT_FOUND;
	err.info = "No symbol called: " + name + " found";
	hx_logger::log_and_exit(err);
	return {};
}

hx_symbol_table* hx_symbol_table::get_symbol_table(const std::string& name)
{
	return symbol_tables.at(name);
}

hx_symbol_table* hx_symbol_table::get_parent()
{
	return parent;
}

hx_symbol_table* hx_symbol_table::get_table_based_on_index(uint32_t index)
{
	return get_symbol_table(std::to_string(index));
}

void hx_symbol_table::print()
{
	printf("SYMBOL TABLE\n");
	for (auto i : symbols)
	{

		printf("%s: ", i.first.c_str());

		switch (i.second.type)
		{
		case hx_symbol_type::VAR:
		{
			printf("var, ");
			break;
		}
		case hx_symbol_type::FUNC:
		{
			printf("func, ");
			break;
		}
		}

		switch (i.second.data_type)
		{
		case hx_data_type::INT:
		{
			printf("int \n");
			break;
		}
		}

	}

	for (auto symbol_table : symbol_tables)
	{
		printf("name: %s\n", symbol_table.first.c_str());
		symbol_table.second->print();
		printf("\n");
	}

}

uint32_t hx_get_size(hx_data_type dt)
{
	switch (dt)
	{
	case hx_data_type::INT: return 8;
	default:
		return 0;
	}
}

void generate_conditional_symbols(hx_symbol_table* table, hx_sptr<hx_conditional_statement> statement, int32_t& relative_stack_pos, uint32_t& index)
{

	
	generate_statement_symbols(table, statement->statement, relative_stack_pos, index, "");

	if (statement->else_statement.has_value())
	{
		generate_statement_symbols(table, statement->else_statement.value(), relative_stack_pos, index, "");
	}

}

void generate_while_symbols(hx_symbol_table* table, hx_sptr<hx_while_statement> statement, int32_t& relative_stack_pos, uint32_t& index)
{

	std::string name = std::to_string(index);
	generate_statement_symbols(table, statement->statement, relative_stack_pos, index, "");
}

void generate_definition_symbols(hx_symbol_table* table, hx_sptr<hx_definition_statement> statement, int32_t& relative_stack_pos)
{
	hx_symbol definition_symbol;
	definition_symbol.data_type = hx_data_type::INT;
	definition_symbol.type = hx_symbol_type::VAR;
	definition_symbol.line_number = statement->line_number;
	relative_stack_pos += hx_get_size(definition_symbol.data_type);
	definition_symbol.stack_position = relative_stack_pos;


	std::string name = (std::dynamic_pointer_cast<hx_definition_statement>(statement))->type_decl->name;
	table->insert(name, definition_symbol);

}

void generate_statement_symbols(hx_symbol_table* table, hx_sptr<hx_statement> statement, int32_t& relative_stack_pos, uint32_t& index, std::string func_name)
{
	
	switch (statement->s_type)
	{
	case statement_type::COMPOUND:
	{
		std::string name;
		if (!func_name.empty())
			name = func_name;
		else
			name = std::to_string(index);
		table->add_symbol_table(name, generate_compound_symbol_table(std::dynamic_pointer_cast<hx_compound_statement>(statement), relative_stack_pos));
		index++;
		break;
	}
	case statement_type::DEFINITION:
	{
		generate_definition_symbols(table, std::dynamic_pointer_cast<hx_definition_statement>(statement), relative_stack_pos);
		break;
	}
	case statement_type::CONDITIONAL:
	{
		generate_conditional_symbols(table, std::dynamic_pointer_cast<hx_conditional_statement>(statement), relative_stack_pos, index);
		break;
	}

	case statement_type::WHILE:
	{
		generate_while_symbols(table, std::dynamic_pointer_cast<hx_while_statement>(statement), relative_stack_pos, index);
		break;
	}

	default:
	{
		break;
	}

	}
}


hx_symbol_table* generate_compound_symbol_table(hx_sptr<hx_compound_statement> compound, int32_t& relative_stack_pos)
{
	hx_symbol_table* table = new hx_symbol_table();
	uint32_t index = 0;
	for (const auto& statement : compound->statements)
	{
		generate_statement_symbols(table, statement, relative_stack_pos, index, "");
	}

	return table;

}



hx_symbol_table* generate_symbol_table(hx_sptr<hx_program> program)
{

	hx_symbol_table* global_table = new hx_symbol_table();
	uint32_t index = 0;
	for (const auto& function : program->functions)
	{
		
		hx_symbol func_symbol;
		func_symbol.data_type = hx_data_type::INT;
		func_symbol.type = hx_symbol_type::FUNC;
		func_symbol.line_number = function->line_number;

		printf("FUNC JÄRJESTYS %s\n", function->name.c_str());

		bool ok = global_table->insert(function->name, func_symbol);

		


		if (!ok)
			printf("ERROR IN SYMBOL TABLE\n");
		//TODO ERROR
		int32_t relative_stack_pos = 0;
		
		generate_statement_symbols(global_table, function->statement, relative_stack_pos, index, function->name);
		

		int32_t param_stack_pos = 0;

		if (function->is_declaration)
			continue;

		hx_symbol_table* func_table = global_table->get_symbol_table(function->name);
	
		
		func_table->allocated_memory_stack = (relative_stack_pos - 8) + (16 - ((relative_stack_pos - 8) % 16)) + 8;

	

		for (const auto param : function->parameters)
		{
			hx_symbol symbol;
			param_stack_pos -= hx_get_size(hx_data_type::INT);
			symbol.stack_position = param_stack_pos - 8; // subtract extra 8 because of push rbp call
			symbol.data_type = hx_data_type::INT;
			symbol.type = hx_symbol_type::VAR;
			symbol.line_number = param->line_number;

			func_table->insert(param->name, symbol);
		}

	}

	return global_table;

}



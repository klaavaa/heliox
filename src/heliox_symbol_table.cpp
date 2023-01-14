#include "heliox_symbol_table.hpp"

bool hx_symbol_table::check_if_exists(const std::string& name)
{
	return (symbols.count(name));
}

bool hx_symbol_table::insert(std::string name, hx_symbol symbol)
{
	if (check_if_exists(name))
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

hx_symbol hx_symbol_table::find_symbol(const std::string& name)
{
	if (check_if_exists(name))
		return symbols.at(name);

	return this->parent->find_symbol(name);
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
	if (index == 0)
		return this;

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

hx_symbol_table* generate_compound_symbol_table(hx_sptr<hx_compound_statement> compound, int32_t& relative_stack_pos, uint32_t depth)
{
	hx_symbol_table* table = new hx_symbol_table();

	int32_t relative_stack_p = relative_stack_pos;
	for (const auto& statement : compound->statements)
	{
		switch (statement->s_type)
		{

		case statement_type::COMPOUND:
		{
			std::string name = std::to_string(depth);
			table->add_symbol_table(name, generate_compound_symbol_table(std::dynamic_pointer_cast<hx_compound_statement>(statement), relative_stack_p, depth + 1));
			break;
		}

		case statement_type::DEFINITION:
		{
			hx_symbol definition_symbol;
			definition_symbol.data_type = hx_data_type::INT;
			definition_symbol.type = hx_symbol_type::VAR;
			relative_stack_p += hx_get_size(definition_symbol.data_type);
			definition_symbol.stack_position = relative_stack_p;
			

			std::string name = (std::dynamic_pointer_cast<hx_definition_statement>(statement))->type_decl->name;
			table->insert(name, definition_symbol);
		}

		}
	}

	return table;



}

hx_symbol_table* generate_symbol_table(hx_sptr<hx_program> program)
{

	hx_symbol_table* global_table = new hx_symbol_table();

	for (const auto& function : program->functions)
	{

		hx_symbol func_symbol;
		func_symbol.data_type = hx_data_type::INT;
		func_symbol.type = hx_symbol_type::FUNC;




		bool ok = global_table->insert(function->name, func_symbol);
		if (!ok)
			printf("ERROR IN SYMBOL TABLE\n");
		//TODO ERROR

		switch (function->statement->s_type)
		{
		case statement_type::COMPOUND:
		{
			int32_t relative_stack_pos = 0;
			global_table->add_symbol_table(function->name, generate_compound_symbol_table(
				std::dynamic_pointer_cast<hx_compound_statement>(function->statement), relative_stack_pos));
			break;
		}

		case statement_type::NOOP:
		{
			global_table->add_symbol_table(function->name, new hx_symbol_table);
			break;
		}
	

		}



	}

	return global_table;

}


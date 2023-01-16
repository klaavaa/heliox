#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "heliox_statement.hpp"
#include "heliox_pointer.hpp"
#include "heliox_error.hpp"

enum class hx_symbol_type : uint32_t
{
	ERR,
	VAR,
	FUNC

};

enum class hx_data_type : uint32_t
{
	VOID,
	INT,
	STRING
};

struct hx_symbol
{
	hx_symbol_type type;
	hx_data_type data_type;
	uint32_t stack_position;
	uint32_t line_number;
};



struct hx_symbol_table
{
public:

	hx_symbol_table() = default;
	
	bool check_if_exists(const std::string& name, hx_symbol_type symbol_type);
	bool insert(std::string name, hx_symbol symbol);
	void add_symbol_table(std::string name, hx_symbol_table* symbol_table);

	
	hx_symbol get_symbol(const std::string& name);
	hx_symbol find_symbol(const std::string& name, hx_symbol_type symbol_type);
	hx_symbol_table*  get_symbol_table(const std::string& name);
	hx_symbol_table* get_parent();
	hx_symbol_table* get_table_based_on_index(uint32_t index);


	void print();

	std::string my_name;

private:
	
	
	std::unordered_map<std::string, hx_symbol> symbols;
	std::unordered_map<std::string, hx_symbol_table*> symbol_tables;

	
	hx_symbol_table* parent;

	

	
};


uint32_t hx_get_size(hx_data_type dt);
hx_symbol_table* generate_compound_symbol_table(hx_sptr<hx_compound_statement> compound, int32_t& relative_stack_pos,uint32_t depth = 1);
hx_symbol_table* generate_symbol_table(hx_sptr<hx_program> program);
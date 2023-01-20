#pragma once

#include <string>
#include <vector>

#include "heliox_pointer.hpp"

enum class expression_type : uint32_t
{
	INT_LITERAL,
	STRING_LITERAL,
	IDENTIFIER_LITERAL,
	FUNCTION_CALL,
	BINOP,

};

struct hx_expression
{
	expression_type e_type;
	uint32_t line_number;
	virtual void print() {};
};


struct hx_int_literal_expression : public hx_expression
{
	hx_int_literal_expression() { e_type = expression_type::INT_LITERAL; }
	int64_t value;

	void print() override
	{
		printf("%lld", value);
	}
};

struct hx_string_literal_expression : public hx_expression
{
	hx_string_literal_expression() { e_type = expression_type::STRING_LITERAL; }
	std::string literal;
	void print() override
	{
		printf("%s\n", literal.c_str());
	}
};
struct hx_identifier_literal_expression : public hx_expression
{
	hx_identifier_literal_expression() { e_type = expression_type::IDENTIFIER_LITERAL; }

	std::string name;

	void print() override
	{
		printf("%s", name.c_str());
	}

};


struct hx_function_call_expression : public hx_expression
{
	hx_function_call_expression() { e_type = expression_type::FUNCTION_CALL; }

	hx_sptr<hx_identifier_literal_expression> identifier;
	std::vector<hx_sptr<hx_expression>> arguments;

	void print() override
	{
		identifier->print();
		for (size_t i = 0; i < arguments.size(); i++)
		{
			printf("argument %zd", i);
			arguments[i]->print();
		}
	}
};


struct hx_binop_expression : public hx_expression
{
	hx_binop_expression() { e_type = expression_type::BINOP; }

	hx_sptr<hx_expression> left;
	hx_sptr<hx_expression> right;
	std::string op;

	void print() override
	{
		printf("(");
		left->print();
		printf(" %s ", op.c_str());
		right->print();
		printf(")");
	}
};


enum class statement_type : uint32_t
{
	STATEMENT,
	COMPOUND,

	RETURN,

	TYPE_DECLARATION,
	DEFINITION,
	
	CONDITIONAL,

	EXPRESSION,
	NOOP
};


struct hx_statement
{
	statement_type s_type;
	uint32_t line_number;
	virtual void print() {};
};

struct hx_compound_statement : public hx_statement
{
	hx_compound_statement() { s_type = statement_type::COMPOUND; }

	std::vector<hx_sptr<hx_statement>> statements;


	void print() override
	{
		printf("{\n");
		for (const auto statement : statements)
		{
			statement->print();
			printf("\n");
		}

		printf("}\n");

	}
};

struct hx_return_statement :public hx_statement
{
	hx_return_statement() { s_type = statement_type::RETURN; }

	hx_sptr<hx_expression> expression;

	void print() override
	{
		printf("return ");
		expression->print();
		printf("\n");
	}

};

struct hx_type_decl_statement : public hx_statement
{
	hx_type_decl_statement() { s_type = statement_type::TYPE_DECLARATION; }

	std::string type;
	std::string name;

	void print() override
	{
		printf("type: %s\tname: %s\n", type.c_str(), name.c_str());
	}
};

struct hx_definition_statement : public hx_statement
{
	hx_definition_statement() { s_type = statement_type::DEFINITION; }

	hx_sptr<hx_type_decl_statement> type_decl;
	hx_sptr<hx_expression> expression;

	void print() override
	{
		type_decl->print();
		printf("equ ");
		expression->print();
	}
	
};

struct hx_conditional_statement : public hx_statement
{
	hx_conditional_statement() { s_type = statement_type::CONDITIONAL; }

	hx_sptr<hx_expression>	expression;
	hx_sptr<hx_statement>	statement;

	void print() override
	{
		printf("IF(");
		expression->print();
		printf(")\n");

		statement->print();
	}

};

struct hx_expression_statement : public hx_statement
{
	hx_expression_statement() { s_type = statement_type::EXPRESSION; }

	hx_sptr<hx_expression> expression;

	void print()
	{
		expression->print();
	}
};

struct hx_noop_statement : public hx_statement
{
	hx_noop_statement() { s_type = statement_type::NOOP; }
};


struct hx_function
{
	std::string name;
	std::vector<hx_sptr<hx_type_decl_statement>> parameters;
	std::string return_type;
	hx_sptr<hx_statement> statement;

	uint32_t line_number;

	void print()
	{
		printf("FUNCTION DECLARATION\nname:%s\n", name.c_str());
		for (uint32_t i = 0; i < parameters.size(); i++)
		{
			printf("%d. param\t", i);
			parameters[i]->print();
		}

		printf("return type: %s\n", return_type.c_str());

		printf("BODY\n");

		statement->print();
	}

};

struct hx_program
{
	std::vector<hx_sptr<hx_function>> functions;

	void print()
	{

		printf("--PROGRAM START--\n\n");
		for (const auto func : functions)
		{
			func->print();
		}

		printf("\n--PROGRAM END--\n");

	}


};











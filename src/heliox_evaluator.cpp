#include "heliox_evaluator.hpp"
#include "heliox_statement.hpp"

std::optional<int64_t> evaluate_int_literal(hx_sptr<hx_int_literal_expression> literal)
{
	return literal->value;
}

std::optional<int64_t> evaluate_unop_expression(hx_sptr<hx_unary_expression> unop)
{
	std::optional<int64_t> value{};


	value = evaluate_expression(unop->expression);

	if (!value.has_value())
		return {};

	tk_type op = tk_type_to_str::get_tk(unop->op.c_str());

	switch (op)
	{

	case TK_MINUS:	return -value.value();
	case TK_PLUS:	return +value.value();
	case TK_NOT:	return !value.value();
	default:
	{
		hx_error err;
		err.ok = false;
		err.line = unop->line_number;
		err.error_type = HX_SYNTAX_ERROR;
		err.info = "Unexpected operator found";
		hx_logger::log_and_exit(err);
		return {};
	}

	}
}

std::optional<int64_t> evaluate_binop_expression(hx_sptr<hx_binop_expression> binop)
{

	std::optional<int64_t> left_value {};
	std::optional<int64_t> right_value{};

	left_value = evaluate_expression(binop->left);
	if (left_value.has_value())
	{
		hx_sptr<hx_int_literal_expression> left_int_literal = make_shared<hx_int_literal_expression>();
		left_int_literal->line_number = binop->line_number;
		left_int_literal->value = left_value.value();
		binop->left = left_int_literal;
	}

	right_value = evaluate_expression(binop->right);
	if (right_value.has_value())
	{
		hx_sptr<hx_int_literal_expression> right_int_literal = make_shared<hx_int_literal_expression>();
		right_int_literal->line_number = binop->line_number;
		right_int_literal->value = right_value.value();
		binop->right = right_int_literal;
	}

	if (!(left_value.has_value() && right_value.has_value()))
		return {};

	tk_type op = tk_type_to_str::get_tk(binop->op.c_str());

	switch (op)
	{
	case TK_PLUS:		 return left_value.value() +  right_value.value();
	case TK_MINUS:		 return left_value.value() -  right_value.value();
	case TK_MULTIPLY:	 return left_value.value() *  right_value.value();
	case TK_DIVIDE:		 return left_value.value() /  right_value.value();
	case TK_GT:			 return left_value.value() >  right_value.value();
	case TK_GTE:		 return left_value.value() >= right_value.value();
	case TK_LT:			 return left_value.value() <  right_value.value();
	case TK_LTE:		 return left_value.value() <= right_value.value();
	case TK_DOUBLE_EQU:  return left_value.value() == right_value.value();
	case TK_NEQU:		 return left_value.value() != right_value.value();
	case TK_LOGICAL_AND: return left_value.value() && right_value.value();
	case TK_LOGICAL_OR:  return left_value.value() || right_value.value();
	case TK_BITWISE_AND: return left_value.value() &  right_value.value();
	case TK_BITWISE_OR:	 return left_value.value() |  right_value.value();
	case TK_BITWISE_XOR: return left_value.value() ^  right_value.value();
	default:
	{
		hx_error err;
		err.ok = false;
		err.line = binop->line_number;
		err.error_type = HX_SYNTAX_ERROR;
		err.info = "Unexpected operator found";
		hx_logger::log_and_exit(err);
		return {};
	}
	}

}

std::optional<int64_t> evaluate_expression(hx_sptr<hx_expression> expression)
{
	switch (expression->e_type)
	{
	case expression_type::BINOP:

		return evaluate_binop_expression(std::dynamic_pointer_cast<hx_binop_expression>(expression));
	case expression_type::INT_LITERAL:
		return evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(expression));

	case expression_type::UNARYOP:
		return evaluate_unop_expression(std::dynamic_pointer_cast<hx_unary_expression>(expression));

	case expression_type::IDENTIFIER_LITERAL:
		return {};

	case expression_type::FUNCTION_CALL:
		return {};

    case expression_type::STRING_LITERAL:
        return {};

	default:
	{
		hx_error err;
		err.ok = false;
		err.line = expression->line_number;
		err.error_type = HX_SYNTAX_ERROR;
		err.info = "Unexpected expression found";
		hx_logger::log_and_exit(err);
		return {};
	}
	}
}

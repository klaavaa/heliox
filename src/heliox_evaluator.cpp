#include "heliox_evaluator.hpp"

int64_t evaluate_int_literal(hx_sptr<hx_int_literal_expression> literal)
{
	return literal->value;
}

int64_t evaluate_binop_expression(hx_sptr<hx_binop_expression> binop)
{
	bool left = false;
	bool right = false;

	int64_t left_value;
	int64_t right_value;

	if (binop->left->e_type == expression_type::BINOP)
		left = true;
	if (binop->right->e_type == expression_type::BINOP)
		right = true;

	if (left)
		left_value = evaluate_binop_expression(std::dynamic_pointer_cast<hx_binop_expression>(binop->left));
	else
		left_value = evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(binop->left));

	if (right)
		right_value = evaluate_binop_expression(std::dynamic_pointer_cast<hx_binop_expression>(binop->right));
	else
		right_value = evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(binop->right));



	if (binop->op == tk_type_to_str::get_str(TK_PLUS))
		return left_value + right_value;
	if (binop->op == tk_type_to_str::get_str(TK_MINUS))
		return left_value - right_value;
	if (binop->op == tk_type_to_str::get_str(TK_MULTIPLY))
		return left_value * right_value;
	if (binop->op == tk_type_to_str::get_str(TK_DIVIDE))
		return left_value / right_value;

}

int64_t evaluate_expression(hx_sptr<hx_expression> expression)
{
	switch (expression->e_type)
	{
	case expression_type::BINOP:
		return evaluate_binop_expression(std::dynamic_pointer_cast<hx_binop_expression>(expression));
	case expression_type::INT_LITERAL:
		return evaluate_int_literal(std::dynamic_pointer_cast<hx_int_literal_expression>(expression));


	}
}

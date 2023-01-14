#pragma once

#include "heliox_statement.hpp"
#include "heliox_token.hpp"


int64_t evaluate_int_literal(hx_sptr<hx_int_literal_expression> literal);
int64_t evaluate_binop_expression(hx_sptr<hx_binop_expression> binop, bool& can_be_evaluated_fully);
int64_t evaluate_expression(hx_sptr<hx_expression> expression, bool& can_be_evaluated_fully);

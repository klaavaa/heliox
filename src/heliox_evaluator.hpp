#pragma once

#include <optional>
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include "heliox_error.hpp"

std::optional<int64_t> evaluate_int_literal(hx_sptr<hx_int_literal_expression> literal);
std::optional<int64_t> evaluate_binop_expression(hx_sptr<hx_binop_expression> binop);
std::optional<int64_t> evaluate_expression(hx_sptr<hx_expression> expression);

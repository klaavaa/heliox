#pragma once
#include "heliox_token.hpp"

namespace hx {

constexpr bool is_valid_binary_operator(tk_type token_type);
constexpr bool is_valid_unary_operator(tk_type token_type);

enum class op_associativity
{
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT
};

constexpr op_associativity get_binop_associativity(tk_type token_type);
constexpr op_associativity get_unop_associativity(tk_type token_type);

constexpr uint32_t get_binop_precedence_level(tk_type token_type);
constexpr uint32_t get_unop_precedence_level(tk_type token_type);

}

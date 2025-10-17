#include "heliox_operator.hpp"
#include "heliox_token.hpp"

namespace hx
{
constexpr bool is_valid_binary_operator(tk_type token_type)
{
    switch (token_type)
    {
    case tk_type::TK_PLUS: 
    case tk_type::TK_MINUS:
    case tk_type::TK_MULTIPLY:
    case tk_type::TK_DIVIDE:
    case tk_type::TK_EQU:
    case tk_type::TK_DOUBLE_EQU:
    case tk_type::TK_NEQU:
    case tk_type::TK_LT:
    case tk_type::TK_GT:
    case tk_type::TK_LTE:
    case tk_type::TK_GTE:
    case tk_type::TK_LOGICAL_AND:
    case tk_type::TK_LOGICAL_OR:
    case tk_type::TK_BITWISE_AND:
    case tk_type::TK_BITWISE_OR:
    case tk_type::TK_BITWISE_XOR:
        return true;



    default:
        return false;
    }
} 

constexpr bool is_valid_unary_operator(tk_type token_type)
{
    switch (token_type)
    {
        case tk_type::TK_PLUS:
        case tk_type::TK_MINUS:
        case tk_type::TK_MULTIPLY:
        case tk_type::TK_BITWISE_AND:
        case tk_type::TK_NOT:
            return true;
    default:
        return false;
    }
}

constexpr uint32_t get_binop_precedence_level(tk_type token_type)
{
	switch (token_type)
	{
        case tk_type::TK_EQU:
            return 2;
        case tk_type::TK_LOGICAL_OR:
            return 4;
        case tk_type::TK_LOGICAL_AND:
            return 5;
        case tk_type::TK_BITWISE_OR:
            return 6;
        case tk_type::TK_BITWISE_XOR:
            return 7;
        case tk_type::TK_BITWISE_AND:
            return 8;
        case tk_type::TK_DOUBLE_EQU:
        case tk_type::TK_NEQU:
            return 9;
        case tk_type::TK_LT:
        case tk_type::TK_GT:
        case tk_type::TK_LTE:
        case tk_type::TK_GTE:
            return 10;
        case tk_type::TK_PLUS: 
        case tk_type::TK_MINUS:
            return 12;
        case tk_type::TK_MULTIPLY:
        case tk_type::TK_DIVIDE:
            return 14;
        
        default:
            return 0; // not a binop
    }
}
constexpr uint32_t get_unop_precedence_level(tk_type token_type)
{
    switch (token_type)
    {
        case tk_type::TK_NOT:
        case tk_type::TK_PLUS:
        case tk_type::TK_MINUS:
        case tk_type::TK_MULTIPLY:
        case tk_type::TK_BITWISE_AND:
            return 15;

        default: // not a un op
            return 0;
    }
}
constexpr op_associativity get_binop_associativity(tk_type token_type)
{

	switch (token_type)
	{
		// 2
	case tk_type::TK_EQU:
        return op_associativity::RIGHT_TO_LEFT;
		// 4
	case tk_type::TK_LOGICAL_OR:
    case tk_type::TK_LOGICAL_AND:
    case tk_type::TK_BITWISE_OR:
    case tk_type::TK_BITWISE_AND:
    case tk_type::TK_DOUBLE_EQU:
    case tk_type::TK_NEQU:
    case tk_type::TK_LT:
    case tk_type::TK_LTE:
    case tk_type::TK_GT:
    case tk_type::TK_GTE:
    case tk_type::TK_PLUS:
    case tk_type::TK_MINUS:
    case tk_type::TK_DIVIDE:
    case tk_type::TK_MULTIPLY:
		return op_associativity::LEFT_TO_RIGHT;

	default: // not a bin op
		return op_associativity::LEFT_TO_RIGHT;

	}

}
constexpr op_associativity get_unop_associativity(tk_type token_type)
{
    switch (token_type)
    {
        case tk_type::TK_NOT:
        case tk_type::TK_PLUS:
        case tk_type::TK_MINUS:
        case tk_type::TK_MULTIPLY:
        case tk_type::TK_BITWISE_AND:
            return op_associativity::RIGHT_TO_LEFT;

        default: // not a un op
            return op_associativity::RIGHT_TO_LEFT;
    }
}
}

#include "heliox_operator.hpp"
#include "heliox_token.hpp"

namespace hx
{
bool is_valid_binary_operator(tk_type token_type)
{
    switch (token_type)
    {
    case tk_type::PLUS: 
    case tk_type::MINUS:
    case tk_type::MULTIPLY:
    case tk_type::DIVIDE:
    case tk_type::EQU:
    case tk_type::DOUBLE_EQU:
    case tk_type::NEQU:
    case tk_type::LT:
    case tk_type::GT:
    case tk_type::LTE:
    case tk_type::GTE:
    case tk_type::LOGICAL_AND:
    case tk_type::LOGICAL_OR:
    case tk_type::BITWISE_AND:
    case tk_type::BITWISE_OR:
    case tk_type::BITWISE_XOR:
        return true;



    default:
        return false;
    }
} 

bool is_valid_unary_operator(tk_type token_type)
{
    switch (token_type)
    {
        case tk_type::PLUS:
        case tk_type::MINUS:
        case tk_type::MULTIPLY:
        case tk_type::BITWISE_AND:
        case tk_type::NOT:
            return true;
    default:
        return false;
    }
}

uint32_t get_binop_precedence_level(tk_type token_type)
{
	switch (token_type)
	{
        case tk_type::EQU:
            return 2;
        case tk_type::LOGICAL_OR:
            return 4;
        case tk_type::LOGICAL_AND:
            return 5;
        case tk_type::BITWISE_OR:
            return 6;
        case tk_type::BITWISE_XOR:
            return 7;
        case tk_type::BITWISE_AND:
            return 8;
        case tk_type::DOUBLE_EQU:
        case tk_type::NEQU:
            return 9;
        case tk_type::LT:
        case tk_type::GT:
        case tk_type::LTE:
        case tk_type::GTE:
            return 10;
        case tk_type::PLUS: 
        case tk_type::MINUS:
            return 12;
        case tk_type::MULTIPLY:
        case tk_type::DIVIDE:
            return 14;
        
        default:
            return 0; // not a binop
    }
}
uint32_t get_unop_precedence_level(tk_type token_type)
{
    switch (token_type)
    {
        case tk_type::NOT:
        case tk_type::PLUS:
        case tk_type::MINUS:
        case tk_type::MULTIPLY:
        case tk_type::BITWISE_AND:
            return 15;

        default: // not a un op
            return 0;
    }
}
op_associativity get_binop_associativity(tk_type token_type)
{

	switch (token_type)
	{
		// 2
	case tk_type::EQU:
        return op_associativity::RIGHT_TO_LEFT;
		// 4
	case tk_type::LOGICAL_OR:
    case tk_type::LOGICAL_AND:
    case tk_type::BITWISE_OR:
    case tk_type::BITWISE_AND:
    case tk_type::DOUBLE_EQU:
    case tk_type::NEQU:
    case tk_type::LT:
    case tk_type::LTE:
    case tk_type::GT:
    case tk_type::GTE:
    case tk_type::PLUS:
    case tk_type::MINUS:
    case tk_type::DIVIDE:
    case tk_type::MULTIPLY:
		return op_associativity::LEFT_TO_RIGHT;

	default: // not a bin op
		return op_associativity::LEFT_TO_RIGHT;

	}

}
op_associativity get_unop_associativity(tk_type token_type)
{
    switch (token_type)
    {
        case tk_type::NOT:
        case tk_type::PLUS:
        case tk_type::MINUS:
        case tk_type::MULTIPLY:
        case tk_type::BITWISE_AND:
            return op_associativity::RIGHT_TO_LEFT;

        default: // not a un op
            return op_associativity::RIGHT_TO_LEFT;
    }
}
}

#include "heliox_operator.hpp"
#include "heliox_token.hpp"

namespace hx
{
bool is_valid_binary_operator(TokenType token_type)
{
    switch (token_type)
    {
    case TokenType::PLUS: 
    case TokenType::MINUS:
    case TokenType::MULTIPLY:
    case TokenType::DIVIDE:
    case TokenType::MODULO:
    case TokenType::PLUSEQUALS:
    case TokenType::MINUSEQUALS:
    case TokenType::DIVEQUALS:
    case TokenType::MULEQUALS:
    case TokenType::EQU:
    case TokenType::DOUBLE_EQU:
    case TokenType::NEQU:
    case TokenType::LT:
    case TokenType::GT:
    case TokenType::LTE:
    case TokenType::GTE:
    case TokenType::LOGICAL_AND:
    case TokenType::LOGICAL_OR:
    case TokenType::BITWISE_AND:
    case TokenType::BITWISE_OR:
    case TokenType::BITWISE_XOR:
        return true;



    default:
        return false;
    }
} 

bool is_valid_unary_operator(TokenType token_type)
{
    switch (token_type)
    {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::BITWISE_AND:
        case TokenType::NOT:
            return true;
    default:
        return false;
    }
}

uint32_t get_binop_precedence_level(TokenType token_type)
{
	switch (token_type)
	{
        case TokenType::EQU:
        case TokenType::PLUSEQUALS:
        case TokenType::MINUSEQUALS:
        case TokenType::DIVEQUALS:
        case TokenType::MULEQUALS:
            return 2;
        case TokenType::LOGICAL_OR:
            return 4;
        case TokenType::LOGICAL_AND:
            return 5;
        case TokenType::BITWISE_OR:
            return 6;
        case TokenType::BITWISE_XOR:
            return 7;
        case TokenType::BITWISE_AND:
            return 8;
        case TokenType::DOUBLE_EQU:
        case TokenType::NEQU:
            return 9;
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LTE:
        case TokenType::GTE:
            return 10;
        case TokenType::PLUS: 
        case TokenType::MINUS:
            return 12;
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::MODULO:
            return 14;
        
        default:
            return 0; // not a binop
    }
}
uint32_t get_unop_precedence_level(TokenType token_type)
{
    switch (token_type)
    {
        case TokenType::NOT:
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::BITWISE_AND:
            return 15;

        default: // not a un op
            return 0;
    }
}
op_associativity get_binop_associativity(TokenType token_type)
{

	switch (token_type)
	{
		// 2
	case TokenType::EQU:
    case TokenType::PLUSEQUALS:
    case TokenType::MINUSEQUALS:
    case TokenType::DIVEQUALS:
    case TokenType::MULEQUALS:
        return op_associativity::RIGHT_TO_LEFT;
		// 4
	case TokenType::LOGICAL_OR:
    case TokenType::LOGICAL_AND:
    case TokenType::BITWISE_OR:
    case TokenType::BITWISE_AND:
    case TokenType::DOUBLE_EQU:
    case TokenType::NEQU:
    case TokenType::LT:
    case TokenType::LTE:
    case TokenType::GT:
    case TokenType::GTE:
    case TokenType::PLUS:
    case TokenType::MINUS:
    case TokenType::DIVIDE:
    case TokenType::MULTIPLY:
		return op_associativity::LEFT_TO_RIGHT;

	default: // not a bin op
		return op_associativity::LEFT_TO_RIGHT;

	}

}
op_associativity get_unop_associativity(TokenType token_type)
{
    switch (token_type)
    {
        case TokenType::NOT:
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::BITWISE_AND:
            return op_associativity::RIGHT_TO_LEFT;

        default: // not a un op
            return op_associativity::RIGHT_TO_LEFT;
    }
}
}

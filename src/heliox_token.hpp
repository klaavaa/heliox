#pragma once

#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <string>
#include <cstdint>
#include <optional>

#define HX_COMMA						','
#define HX_DOT							'.'
#define HX_DOLLAR						'$'

#define HX_AMPERSAND					'&'
#define HX_PIPE						    '|'
#define HX_CIRCUMFLEX					'^'

#define HX_EXCLAMATION_MARK		    	'!'
#define HX_QUESTION_MARK				'?'

#define HX_PLUS					    	'+'
#define HX_MINUS						'-'
#define HX_DIVIDE						'/'
#define HX_STAR					    	'*'

#define HX_EQUALS						'='


#define HX_LEFT_BRACE					'{'
#define HX_RIGHT_BRACE					'}'
#define HX_LEFT_PAREN					'('
#define HX_RIGHT_PAREN					')'
#define HX_LEFT_BRACK					'['
#define HX_RIGHT_BRACK					']'

#define HX_SEMICOLON					';'
#define HX_COLON						':'

#define HX_QUOT_MARK					'"'

#define HX_AT							'@'

#define HX_LEFT_ARROW					'<'
#define HX_RIGHT_ARROW					'>'

#define HX_SPACE						' '
#define HX_TAB							'\t'
#define HX_NEWLINE						'\n'
#define HX_EOF                          (-1)

namespace hx {
constexpr const char* characters = "abcdefghijklmnopqrstuvwxyz���ABCDEFGHIJKLMNOPQRSTUVWXYZ���_�";
constexpr const char* numbers = "0123456789";
constexpr const char* characters_numbers = "0123456789abcdefghijklmnopqrstuvwxyz���ABCDEFGHIJKLMNOPQRSTUVWXYZ���_�";


enum class TokenType
{

	END_OF_FILE = 0,

	NOT_A_TOKEN,

	KEYWORD,
	IDENTIFIER,

	COMMA,
	DOT,
	QUOT_MARK,

	SEMICOLON,
	COLON,

	PLUS,
	MINUS,
	DIVIDE,
	MULTIPLY,

	L_PAREN,
	R_PAREN,

	L_BRACE,
	R_BRACE,

	L_BRACK,
	R_BRACK,

	DOLLAR,
	BITWISE_AND,
	BITWISE_OR,
	BITWISE_XOR,
	EQU,
	NOT,
	INTEGER,
	FLOAT,
	STRING,

	AT,

	LOGICAL_AND,
	LOGICAL_OR,
	GT,
	GTE,
	LT,
	LTE,
	DOUBLE_EQU,
	NEQU,

	ARROW


};

static const std::unordered_map<TokenType, std::string>  token_to_string_map =
{

{ TokenType::END_OF_FILE,	    				"EOF" },
{ TokenType::KEYWORD,							"KEYWORD" },
{ TokenType::IDENTIFIER,						"IDENTIFIER" },

{ TokenType::COMMA,							"COMMA" },
{ TokenType::DOT,								"DOT" },
{ TokenType::QUOT_MARK,                       "QUOT_MARK"},
{ TokenType::SEMICOLON,						"SEMICOLON" },
{ TokenType::COLON,							"COLON" },
{ TokenType::PLUS,							"PLUS" },
{ TokenType::MINUS,							"MINUS" },
{ TokenType::DIVIDE,							"SLASH" },
{ TokenType::MULTIPLY,						"STAR" },
{ TokenType::L_PAREN,							"LPAREN" },
{ TokenType::R_PAREN,							"RPAREN" },
{ TokenType::L_BRACE,							"LBRACE" },
{ TokenType::R_BRACE,							"RBRACE" },
{ TokenType::L_BRACK,							"LBRACK" },
{ TokenType::R_BRACK,							"RBRACK" },
{ TokenType::DOLLAR ,							"DOLLAR" },
{ TokenType::AT ,								"AT" },
{ TokenType::BITWISE_AND,						"AMPERSAND" },
{ TokenType::BITWISE_OR ,						"PIPE" },
{ TokenType::BITWISE_XOR ,					"HAT" },
{ TokenType::LOGICAL_AND,						"AND"},
{ TokenType::LOGICAL_OR,						"OR"},
{ TokenType::EQU,								"EQUALS" },
{ TokenType::DOUBLE_EQU,						"EQUAL_COMPARISON"},
{ TokenType::NOT,								"EXCLAMATION" },
{ TokenType::NEQU,							"NOT EQUALS"},
{ TokenType::INTEGER,							"INTEGER" },
{ TokenType::FLOAT,							"FLOAT" },
{ TokenType::STRING,							"STRING"},
{ TokenType::LT,								"LESS THAN" },
{ TokenType::LTE,								"LESS THAN OR EQUAL"}, 
{ TokenType::GTE,								"GREATER THAN OR EQUAL"},
{ TokenType::GT,								"GREATER THAN" },
{ TokenType::ARROW,							"ARROW"}				
};

constexpr std::string get_string_from_token_type(TokenType type)
{
    if (!token_to_string_map.count(type)) return "COULDNT MATCH TOKEN WITH STRING";
    return token_to_string_map.find(type)->second;
}

struct Token
{
    Token() : type(TokenType::NOT_A_TOKEN), value()
    {
    }
	Token(TokenType tok_type, std::string tok_value)
		:
		type(tok_type),
		value(tok_value)
	{}



	TokenType type;
	std::string value;

};
}



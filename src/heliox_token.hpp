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
constexpr const char* characters = "abcdefghijklmnopqrstuvwxyzåäöABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ_§";
constexpr const char* numbers = "0123456789";
constexpr const char* characters_numbers = "0123456789abcdefghijklmnopqrstuvwxyzåäöABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ_§";


enum class tk_type
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

static const std::unordered_map<tk_type, std::string>  token_to_string_map =
{

{ tk_type::END_OF_FILE,	    				"EOF" },
{ tk_type::KEYWORD,							"KEYWORD" },
{ tk_type::IDENTIFIER,						"IDENTIFIER" },

{ tk_type::COMMA,							"COMMA" },
{ tk_type::DOT,								"DOT" },
{ tk_type::QUOT_MARK,                       "QUOT_MARK"},
{ tk_type::SEMICOLON,						"SEMICOLON" },
{ tk_type::COLON,							"COLON" },
{ tk_type::PLUS,							"PLUS" },
{ tk_type::MINUS,							"MINUS" },
{ tk_type::DIVIDE,							"SLASH" },
{ tk_type::MULTIPLY,						"STAR" },
{ tk_type::L_PAREN,							"LPAREN" },
{ tk_type::R_PAREN,							"RPAREN" },
{ tk_type::L_BRACE,							"LBRACE" },
{ tk_type::R_BRACE,							"RBRACE" },
{ tk_type::L_BRACK,							"LBRACK" },
{ tk_type::R_BRACK,							"RBRACK" },
{ tk_type::DOLLAR ,							"DOLLAR" },
{ tk_type::AT ,								"AT" },
{ tk_type::BITWISE_AND,						"AMPERSAND" },
{ tk_type::BITWISE_OR ,						"PIPE" },
{ tk_type::BITWISE_XOR ,					"HAT" },
{ tk_type::LOGICAL_AND,						"AND"},
{ tk_type::LOGICAL_OR,						"OR"},
{ tk_type::EQU,								"EQUALS" },
{ tk_type::DOUBLE_EQU,						"EQUAL_COMPARISON"},
{ tk_type::NOT,								"EXCLAMATION" },
{ tk_type::NEQU,							"NOT EQUALS"},
{ tk_type::INTEGER,							"INTEGER" },
{ tk_type::FLOAT,							"FLOAT" },
{ tk_type::STRING,							"STRING"},
{ tk_type::LT,								"LESS THAN" },
{ tk_type::LTE,								"LESS THAN OR EQUAL"}, 
{ tk_type::GTE,								"GREATER THAN OR EQUAL"},
{ tk_type::GT,								"GREATER THAN" },
{ tk_type::ARROW,							"ARROW"}				
};

constexpr std::string get_string_from_token_type(tk_type type)
{
    if (!token_to_string_map.count(type)) return "COULDNT MATCH TOKEN WITH STRING";
    return token_to_string_map.find(type)->second;
}

struct token
{
    token() : type(tk_type::NOT_A_TOKEN), value()
    {
    }
	token(tk_type tok_type, std::string tok_value)
		:
		type(tok_type),
		value(tok_value)
	{}



	tk_type type;
	std::string value;

};
}



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

static const std::unordered_map<tk_type, const char*>  token_to_string =
{

{ tk_type::END_OF_FILE,	    				"EOF" },
{ tk_type::KEYWORD,							"KEYWORD" },
{ tk_type::IDENTIFIER,						"IDENTIFIER" },

{ tk_type::COMMA,							"," },
{ tk_type::DOT,								"." },
{ tk_type::QUOT_MARK,                        "\""},
{ tk_type::SEMICOLON,						";" },
{ tk_type::COLON,							":" },
{ tk_type::PLUS,								"+" },
{ tk_type::MINUS,							"-" },
{ tk_type::DIVIDE,							"/" },
{ tk_type::MULTIPLY,							"*" },
{ tk_type::L_PAREN,							"(" },
{ tk_type::R_PAREN,							")" },
{ tk_type::L_BRACE,							"{" },
{ tk_type::R_BRACE,							"}" },
{ tk_type::L_BRACK,							"[" },
{ tk_type::R_BRACK,							"]" },
{ tk_type::DOLLAR ,							"$" },
{ tk_type::AT ,								"@" },
{ tk_type::BITWISE_AND,						"&" },
{ tk_type::BITWISE_OR ,						"|" },
{ tk_type::BITWISE_XOR ,						"^" },
{ tk_type::LOGICAL_AND,						"&&"},
{ tk_type::LOGICAL_OR,						"||"},
{ tk_type::EQU,								"=" },
{ tk_type::DOUBLE_EQU,						"=="},
{ tk_type::NOT,								"!" },
{ tk_type::NEQU,								"!="},
{ tk_type::INTEGER,							"INTEGER" },
{ tk_type::FLOAT,							"FLOAT" },
{ tk_type::STRING,							"STRING"},
{ tk_type::LT,								"<" },
{ tk_type::LTE,								"<="}, 
{ tk_type::GTE,								">="},
{ tk_type::GT,								">" },
{ tk_type::ARROW,							"->"}				


};

/*
struct tk_type_to_str
{

	tk_type_to_str() = delete;

	static const char* get_str(tk_type tok_type)
	{
		if (!token_to_string.count(tok_type)) return "COULDNT MATCH TOKEN WITH STRING";
		return token_to_string.find(tok_type)->second;
	}


	static tk_type get_tk(const char* str)
	{
		static std::unordered_map<const char*, tk_type> string_to_token = get_reversed();


		auto it = std::find_if(string_to_token.begin(), string_to_token.end(), [str](const std::pair<const char*, tk_type>& t)->bool
			{return !strcmp(str, t.first); });

		if (it == string_to_token.end()) return tk_type::NOT_A_TOKEN;

		return it->second;
	}

private:

	static std::unordered_map<const char*, tk_type> get_reversed()
	{
		std::unordered_map<const char*, tk_type> string_to_token;
		for (const auto& tok : token_to_string)
		{
			string_to_token[tok.second] = tok.first;
		}
	
		return string_to_token;
		
	}
};
*/
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



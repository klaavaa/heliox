#pragma once
#include <iostream>
#include <unordered_map>


#define COMMA						','
#define DOT							'.'
#define DOLLAR						'$'

#define AMPERSAND					'&'
#define PIPE						'|'
#define CIRCUMFLEX					'^'

#define EXCLAMATION_MARK			'!'
#define QUESTION_MARK				'?'

#define PLUS						'+'
#define MINUS						'-'
#define DIVIDE						'/'
#define STAR						'*'

#define EQUALS						'='


#define LEFT_BRACE					'{'
#define RIGHT_BRACE					'}'
#define LEFT_PAREN					'('
#define RIGHT_PAREN					')'
#define LEFT_BRACK					'['
#define RIGHT_BRACK					']'

#define SEMICOLON					';'
#define COLON						':'

#define QUOT_MARK					'"'

#define AT							'@'

#define LEFT_ARROW					'<'
#define RIGHT_ARROW					'>'

#define SPACE						' '
#define TAB							'\t'
#define NEWLINE						'\n'



constexpr const char* characters = "abcdefghijklmnopqrstuvwxyzåäöABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ_§";
constexpr const char* numbers = "0123456789";
constexpr const char* characters_numbers = "0123456789abcdefghijklmnopqrstuvwxyzåäöABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ_§";



enum tk_type : uint32_t
{

	TK_EOF = 0,

	TK_NOT_A_TOKEN,

	TK_KEYWORD,
	TK_IDENTIFIER,

	TK_COMMA,
	TK_DOT,
	TK_QUOT_MARK,

	TK_SEMICOLON,
	TK_COLON,

	TK_PLUS,
	TK_MINUS,
	TK_DIVIDE,
	TK_MULTIPLY,

	TK_L_PAREN,
	TK_R_PAREN,

	TK_L_BRACE,
	TK_R_BRACE,

	TK_L_BRACK,
	TK_R_BRACK,

	TK_DOLLAR,
	TK_AND,
	TK_OR,
	TK_XOR,
	TK_EQU,
	TK_NOT,
	TK_INTEGER,
	TK_FLOAT,
	TK_STRING,
	TK_AT,

	TK_GT,
	TK_LT,

	TK_ARROW


};

static const std::unordered_map<tk_type, const char*>  token_to_string =
{

{ tk_type::TK_EOF,								"EOF" },
{ tk_type::TK_KEYWORD,							"KEYWORD" },
{ tk_type::TK_IDENTIFIER,						"IDENTIFIER" },

{ tk_type::TK_COMMA,							"," },
{ tk_type::TK_DOT,								"." },
{ tk_type::TK_QUOT_MARK,                        "\""},
{ tk_type::TK_SEMICOLON,						";" },
{ tk_type::TK_COLON,							":" },
{ tk_type::TK_PLUS,								"+" },
{ tk_type::TK_MINUS,							"-" },
{ tk_type::TK_DIVIDE,							"/" },
{ tk_type::TK_MULTIPLY,							"*" },
{ tk_type::TK_L_PAREN,							"(" },
{ tk_type::TK_R_PAREN,							")" },
{ tk_type::TK_L_BRACE,							"{" },
{ tk_type::TK_R_BRACE,							"}" },
{ tk_type::TK_L_BRACK,							"[" },
{ tk_type::TK_R_BRACK,							"]" },
{ tk_type::TK_DOLLAR ,							"$" },
{ tk_type::TK_AT ,								"@" },
{ tk_type::TK_AND,								"&" },
{ tk_type::TK_OR ,								"|" },
{ tk_type::TK_XOR ,								"^" },
{ tk_type::TK_EQU,								"=" },
{ tk_type::TK_NOT,								"!" },
{ tk_type::TK_INTEGER,							"INTEGER" },
{ tk_type::TK_FLOAT,							"FLOAT" },
{ tk_type::TK_STRING,							"STRING"},
{ tk_type::TK_LT,								"<" },
{ tk_type::TK_GT,								">" },
{ tk_type::TK_ARROW,							"->"}				


};


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

		if (it == string_to_token.end()) return tk_type::TK_NOT_A_TOKEN;

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


struct hx_token
{

	hx_token(tk_type tok_type, std::string tok_value)
		:
		type(tok_type),
		value(tok_value)
	{}



	tk_type type;
	std::string value;

};
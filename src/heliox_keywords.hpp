#pragma once
#include <iostream>

#include <string>
#include <unordered_map>


enum class hx_kwords : uint32_t
{
	ERR = 0,
	INT = 1,
	FLOAT,
	STRING,
	FUNC,
	VOID,
	RETURN,
	IF,
	ELSE

};

struct hx_keyword
{

	enum hx_kword_type : uint32_t
	{
		ERROR,
		TYPE_DECL,
		STATEMENT
	};

	hx_keyword(hx_kword_type t, hx_kwords kword)
		:
		type(t),
		keyword(kword)
	{

	}

	hx_kword_type type;
	hx_kwords keyword;

};


static const std::unordered_map<std::string, hx_keyword> keywords = {

	/* ====ERROR==== */
	{"",			{hx_keyword::ERROR, hx_kwords::ERR}},

	/* ====TYPE DECL==== */
	{"int",			{hx_keyword::TYPE_DECL, hx_kwords::INT}	},
	{"i32",			{hx_keyword::TYPE_DECL, hx_kwords::INT}	},
	{"float",		{hx_keyword::TYPE_DECL, hx_kwords::FLOAT}	},
	{"string",		{hx_keyword::TYPE_DECL, hx_kwords::STRING}},
	{"void",		{hx_keyword::TYPE_DECL, hx_kwords::VOID} },
	/* ====STATEMENT==== */
	{"func",		{hx_keyword::STATEMENT, hx_kwords::FUNC}},
	{"return",		{hx_keyword::STATEMENT, hx_kwords::RETURN}},
	{"if",          {hx_keyword::STATEMENT, hx_kwords::IF}},
	{"else",		{hx_keyword::STATEMENT, hx_kwords::ELSE}}
};

inline hx_keyword get_kword_from_string(const std::string& name)
{

	if (!keywords.count(name))
		return { hx_keyword::ERROR, hx_kwords::ERR };
	return keywords.at(name);

}


inline std::string get_string_from_kword(const hx_kwords kword)
{

	std::unordered_map<hx_kwords, std::string> reversed;

	for ( const auto& kw : keywords)
	{
		reversed[kw.second.keyword] = kw.first;
	}

	return reversed[kword];

}







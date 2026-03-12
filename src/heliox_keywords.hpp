#pragma once
#include <iostream>

#include <string>
#include <unordered_map>
#include <cstdint>
#include "heliox_error.hpp"

namespace hx {
    
enum class keyword : uint32_t
{
	FUN,
	VOID,
	RETURN,
	IF,
	ELSE,
	WHILE,
    EXTERN,
};


inline const std::unordered_map<std::string, keyword> keywords = {
	{"fun",			 keyword::FUN},
	{"return",		 keyword::RETURN},
	{"if",           keyword::IF},
	{"else",		 keyword::ELSE},
	{"while",		 keyword::WHILE},
    {"extern",       keyword::EXTERN},
};

inline keyword get_kword_from_string(const std::string& name)
{
	if (!keywords.count(name))
    {
        hx::Error error;
        error.error_type = HX_SYNTAX_ERROR;
        error.info = "unknown keyword: '" + name + "'";
        hx::Logger::log_and_exit(error);
    }
    return keywords.at(name);

}


inline std::string get_string_from_kword(const keyword keyword)
{

    for (const auto& [key, value] : keywords)
        if (value == keyword)
            return key;

    // if this case is reached then the fabric of the universe has teared
    // so return empty string i guess?
    return {};
}
}



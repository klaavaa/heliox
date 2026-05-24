#pragma once
#include <iostream>

#include <string>
#include <unordered_map>
#include <cstdint>
#include <print>
#include "heliox_error.hpp"

namespace hx {
    
enum class KeyWord : uint32_t
{
	FUN,
	VOID,
	RETURN,
	IF,
	ELSE,
	WHILE,
    EXTERN,
    MODULE,
    IMPORT
};


inline const std::unordered_map<std::string_view, KeyWord> keywords = {
	{"fun",			 KeyWord::FUN},
	{"return",		 KeyWord::RETURN},
	{"if",           KeyWord::IF},
	{"else",		 KeyWord::ELSE},
	{"while",		 KeyWord::WHILE},
    {"extern",       KeyWord::EXTERN},
	{"module",		 KeyWord::MODULE},
    {"import",       KeyWord::IMPORT},
};

inline KeyWord get_kword_from_string(std::string_view name)
{
	if (!keywords.count(name))
    {
        //TODO ERROR
        std::println("Unexpected keyword '{}' at get_kword_from_string", name);
        exit(-1);
    }
    return keywords.at(name);

}


inline std::string_view get_string_from_kword(const KeyWord keyword)
{

    for (const auto& [key, value] : keywords)
        if (value == keyword)
            return key;


    
    // if this case is reached then the fabric of the universe has teared
    // so return empty string i guess?
    // TODO ERROR
    std::println(stderr, "Unexpected keyword at get_string_from_kword");
    exit(-1);

    return {};
}
}



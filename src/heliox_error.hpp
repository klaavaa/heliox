#pragma once
#include <string>
#include <cstdint>
#include <print>
#include "heliox_tools.hpp"
#include "heliox_token.hpp"
#include "heliox_ast_node.hpp"

#define HX_TODO 999

#define HX_NOT_HELIOX_FILE 1
#define HX_FILE_OPEN_ERROR 2

#define HX_UNRECONIZED_CHARACTER 10
#define HX_UNTERMINATED_STRING_LITERAL 11 
#define HX_UNTERMINATED_MULTILINE_COMMENT 12 
#define HX_INVALID_FLOAT_LITERAL 13

#define HX_VARARGS_NOT_LAST_ARG 99
#define HX_EXTERN_FUNC_WITH_BODY 100
#define HX_NOT_PRIMITIVE_TYPE 101
#define HX_UNEXPECTED_TOKEN 102
#define HX_UNEXPECTED_KEYWORD 103

#define HX_SYMBOL_REDEFINITION 200
#define HX_SYMBOL_NOT_FOUND 201
#define HX_MODULE_NOT_FOUND 202

#define HX_INVALID_ARGUMENTS 300


namespace hx 
{


class Logger
{
public:
	[[noreturn]] static void error(std::string_view filename, int error_code, std::string_view info);
	[[noreturn]] static void error(std::string_view filename, uint32_t line, uint32_t position, int error_code, std::string_view info);
	[[noreturn]] static void error(const Token& token, int error_code,std::string_view info);	
	[[noreturn]] static void error(const ast_node& node, int error_code, std::string_view info);

	static void warning(Token& token, int warning_code,std::string_view info);

	template<typename... Args>
	static void info(std::string_view info, Args&&... args)
	{
		auto formatted_info = std::vformat(info, std::make_format_args(std::forward<Args>(args)...));
		std::println(stdout, "[info]: {}", formatted_info);
	}

};
}

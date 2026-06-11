#pragma once
#include <cstdint>
#include <print>
#include <source_location>
#include "heliox_token.hpp"
#include "heliox_ast_node.hpp"

namespace hx 
{


class Logger
{
public:
	[[noreturn]] static void error(std::string_view filename, std::string_view info);
	[[noreturn]] static void error(std::string_view filename, uint32_t line, uint32_t position, std::string_view info);
	[[noreturn]] static void error(const Token& token, std::string_view info);	
	[[noreturn]] static void error(const ast_node& node, std::string_view info);


	template<typename... Args>
	[[noreturn]] static void pre_compile_error(std::string_view info, Args&&... args)
	{
		auto formatted_info = std::vformat(info, std::make_format_args(std::forward<Args>(args)...));
		std::println(stderr, "error: {}", formatted_info);
		exit(-1);
	}


	static void warning(Token& token, std::string_view info);

	[[noreturn]] static void not_implemented(std::source_location location = std::source_location::current())
	{
		std::println(stderr, "{}({}:{}): {}: NOT IMPLEMENTED", location.file_name(), location.line(), location.column(), location.function_name());
		exit(-1);
	}
	[[noreturn]] static void internal_error(std::source_location location = std::source_location::current())
	{
		std::println(stderr, "{}({}:{}): {}: INTERNAL ERROR", location.file_name(), location.line(), location.column(), location.function_name());
		exit(-1);
	}
	[[noreturn]] static void todo_error(std::source_location location = std::source_location::current())
	{
		std::println(stderr, "{}({}:{}): {}: TODO ERROR", location.file_name(), location.line(), location.column(), location.function_name());
		exit(-1);
	}

	template<typename... Args>
	static void info(std::string_view info, Args&&... args)
	{
		auto formatted_info = std::vformat(info, std::make_format_args(std::forward<Args>(args)...));
		std::println(stdout, "[info]: {}", formatted_info);
	}

};
}

#include "heliox_error.hpp"
#include <print>

namespace hx {

void Logger::error(std::string_view filename, std::string_view info)
{
	std::println(stderr, "{}(): error: {}", filename, info);
	exit(1);
}

void Logger::error(std::string_view filename, uint32_t line, uint32_t position, std::string_view info)
{
	std::println(stderr, "{}({}:{}): error: {}", filename, line, position, info);
	exit(1);
}

void Logger::error(const Token& token, std::string_view info)
{
	error(token.filename, token.line, token.position, info);
}
	
void Logger::error(const ast_node& node, std::string_view info)
{
	error(node.filename, node.line, node.position, info);
}

void Logger::warning(Token& token, std::string_view info)
{
	std::println(stderr, "{}({}:{}): warning: {}", token.filename, token.line, token.position, info);
}



} //namespace hx

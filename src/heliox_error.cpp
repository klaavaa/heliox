#include "heliox_error.hpp"
#include "heliox_token.hpp"
#include <print>

namespace hx {

void Logger::error(std::string_view filename, int error_code, std::string_view info)
{
	std::println(stderr, "{}(): error {}: {}", filename, error_code, info);
	exit(error_code);
}

void Logger::error(std::string_view filename, uint32_t line, uint32_t position, int error_code, std::string_view info)
{
	std::println(stderr, "{}({}:{}): error {}: {}", filename, line, position, error_code, info);
	exit(error_code);
}

void Logger::error(Token& token, int error_code, std::string_view info)
{
	std::println(stderr, "{}({}:{}): error {}: {}", token.filename, token.line, token.position, error_code, info);
	exit(error_code);
}
void Logger::warning(Token& token, int warning_code, std::string_view info)
{
	std::println(stderr, "{}({}:{}): warning {}: {}", token.filename, token.line, token.position, warning_code, info);
}

}

#pragma once
#include <string>
#include <cstdint>
#include "heliox_tools.hpp"

#define HX_UNDEFINED_ERROR "UndefinedError"
#define HX_UNKNOWN_CHARACTER_ERROR "UnknownCharacterError"
#define HX_SYNTAX_ERROR "SyntaxError"
#define HX_NO_MAIN_ERROR "NoMainFuncError"
#define HX_FILE_OPEN_ERROR "FileOpenError"
#define HX_NOT_HELIOX_FILE "NotHelioxFileError"
#define HX_SYMBOL_NOT_FOUND "SymbolNotFoundError"

namespace hx 
{
struct Error
{
	std::string error_type;
	std::string info;
	std::string file;
	uint32_t line;
};

class Logger
{
public:

	
	static std::string format_error(hx::Error error_data);
	static void log_error(hx::Error error_data);
	static void log_and_exit(hx::Error error_data);

};
}

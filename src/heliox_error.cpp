#include "heliox_error.hpp"

namespace hx {
void logger::log_error(error error_data)
{
	std::string formatted_string = format_error(error_data);
	printf(formatted_string.c_str());

}

void logger::log_and_exit(error error_data)
{
    log_error(error_data);
	exit(-1);
}


std::string logger::format_error(error error_data)
{
	std::string error_string;
	error_string +=
		string_format(
			"File \"%s\", line %d\n",
			error_data.file.c_str(), error_data.line);

	error_string += error_data.error_type + ": " + error_data.info + "\n";

	return error_string;
}

}

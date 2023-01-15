#include "heliox_error.hpp"

void hx_logger::log_error(hx_error error_data)
{
	std::string formatted_string = format_error(error_data);
	printf(formatted_string.c_str());

}


std::string hx_logger::format_error(hx_error error_data)
{
	std::string error_string;
	error_string +=
		string_format(
			"File \"%s\", line %d\n",
			error_data.file.c_str(), error_data.line);

	error_string += error_data.error_type + ": " + error_data.info + "\n";

	return error_string;
}





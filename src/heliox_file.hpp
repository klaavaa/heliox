#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "heliox_error.hpp"
#include "heliox_pointer.hpp"

std::string load_hx_file(const std::string& file_path)
{

	std::ofstream hx_file;
	hx_file.open(file_path, std::ios::in);
	std::stringstream buffer;
	if (hx_file.is_open())
	{
		buffer << hx_file.rdbuf();

	}
	else
	{
        hx::error error;
		error.error_type = HX_FILE_OPEN_ERROR;
		error.line = 0;
		error.info = "Error opening file for reading";
        hx::logger::log_and_exit(error);
	}
	
	return buffer.str();


}


void create_assembly_file(const std::string& file_path, const std::string& assembly_code)
{


	std::string output_file = file_path;
	output_file.append(".asm");


	std::ofstream s_file;
	
	s_file.open(output_file, std::ios::out);

	if (s_file.is_open())
	{
		s_file.write(assembly_code.c_str(), assembly_code.size());

	}
	else
	{
        hx::error error;
		error.error_type = HX_FILE_OPEN_ERROR;
		error.line = 0;
		error.info = "Error opening file for writing";
        hx::logger::log_and_exit(error);
	}
}


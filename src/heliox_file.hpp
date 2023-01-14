#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

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
		std::cout << "ERROR: opening file: " << file_path << std::endl;
		exit(1);
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
		std::cout << "ERROR: opening file for writing" << std::endl;
		exit(1); 
	}


}


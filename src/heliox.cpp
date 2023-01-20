#include <iostream>
#include <cassert>
#include <filesystem>
#include "heliox_file.hpp"
#include "heliox_assembly.hpp"
#include "heliox_error.hpp"



void hx_compile(const std::string& file_path, const std::string& output_path)
{
	hx_sptr<hx_error> error = make_shared<hx_error>();

	if (file_path.substr(file_path.size() - 3) != ".hx")
	{
		error->ok = false;
		error->error_type = HX_NOT_HELIOX_FILE;
		error->line = 0;
		error->info = "Not a heliox file (.hx)";
		hx_logger::log_error(*error);
		exit(1);
	}

	// get last part of absolute path (example home/dir1/dir2/file.hx -> file.hx)
	std::string file_path_stripped = file_path.substr(file_path.find_last_of("/") + 1, file_path.size());

	error->file = file_path_stripped;

	// strip file extension (example file.hx -> file)
	file_path_stripped = file_path_stripped.substr(0, file_path_stripped.size() - 3);


	std::string text = load_hx_file(file_path, error);


	hx_lexer lexer = hx_lexer(text);
	hx_parser parser(&lexer, error);
	hx_sptr<hx_program> program = parser.parse(error);

	if (!error->ok)
	{
		hx_logger::log_error(*error);
		exit(-1);
	}
	program->print();

	bool main_exists = false;
	for (const auto funcs : program->functions)
	{
		if (funcs->name == "main")
			main_exists = true;
	}

	if (!main_exists)
	{
		error->ok = false;
		error->error_type = HX_NO_MAIN_ERROR;
		error->line = 0;
		error->info = "No main function found";
		hx_logger::log_error(*error);
		exit(-1);
	}


	hx_assembly generator;
	std::string generated_text = generator.generate_asm(program);

	std::cout << generated_text << std::endl;


	create_assembly_file(output_path + file_path_stripped, generated_text, error);

}

int main(int argc, char** argv)
{

	std::string file_path = "assembly_build/heliox.hx";
	
	hx_compile(file_path, "linux/");

	


	return 0;
}
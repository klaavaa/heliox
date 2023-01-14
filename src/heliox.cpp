#include <iostream>
#include <cassert>

#include "heliox_file.hpp"
#include "heliox_assembly.hpp"



int main(int argc, char** argv)
{
	std::string file_path = "heliox.hx";

	if (file_path.substr(file_path.size() - 3) != ".hx")
	{
		std::cout << "file not heliox (.hx) file" << std::endl;
		exit(1);
	}

	std::string file_path_stripped = file_path.substr(0, file_path.size() - 3);
	

	std::string text = load_hx_file(file_path);


	hx_lexer lexer = hx_lexer(text);
	hx_parser parser(&lexer);

	hx_sptr<hx_program> program = parser.parse();
	program->print();

	bool main_exists = false;

	for (const auto funcs : program->functions)
	{
		if (funcs->name == "main")
			main_exists = true;
	}

	if (!main_exists)
	{
		printf("ERROR, NO MAIN FUNCTION");
		exit(-1);
	}

	

	hx_assembly generator;
	std::string generated_text = generator.generate_asm(program);

	std::cout << generated_text << std::endl;



	/*
	//compound_s* compound = (compound_s*)parser.parse();
	//std::string assembly = h_assembly_entry(compound);


	//create_assembly_file(file_path_stripped, assembly);


	

	std::cout << assembly << std::endl;

	compound->print_statement();
	

	
	std::cin.get();

	compile_assembly(file_path_stripped);

	std::cin.get();

	*/

	return 0;
}
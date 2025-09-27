#include <iostream>
#include <filesystem>
#include <map>
#include <string>
#include "heliox_file.hpp"
#include "heliox_assembly.hpp"
#include "heliox_error.hpp"
#include "heliox_timer.hpp"
#include "heliox_flags.hpp"

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

    if (HX_IS_FLAG("--debug") || HX_IS_FLAG("-d"))
    {
        program->print();
    }

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

    //	std::cout << generated_text << std::endl;


    create_assembly_file(output_path + file_path_stripped, generated_text, error);

    
    system(("./compile.sh " + file_path_stripped).c_str());
}

int main(int argc, char** argv)
{

    if (argc <= 1)
        return 0;

    hx_flags& flags = HX_GET_FLAGS();

    std::string file_path = ""; 
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            flags[argv[i]] = true;
            continue;
        }
        file_path = argv[i];
    }

    if (file_path.empty())
    {
        std::cout << "Please specify the file you want to compile" << "\n";
        return 0;
    }
    
    auto func = fn<void, const std::string&, const std::string&>(hx_compile);
    double time = hx_timeit<void, const std::string&, const std::string&>(func, file_path, "./");
    std::cout << "Compile time: " << time << "s\n";


    return 0;
}

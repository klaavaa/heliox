#pragma once

#include <string>
#include <print>

#include "heliox_lexer.hpp"
#include "heliox_parser.hpp"
#include "heliox_error.hpp"
#include "heliox_file.hpp"

#include "heliox_debug_visitor.hpp"
#include "heliox_instruction_gen.hpp"



namespace hx  
{
inline void compile(const std::string& file_path, const std::string& output_path)
{
    
    if (file_path.substr(file_path.size() - 3) != ".hx")
    {
        hx::error error;
        error.error_type = HX_NOT_HELIOX_FILE;
        error.line = 0;
        error.info = "Not a heliox file (.hx)";
        hx::logger::log_error(error);
        exit(1);
    }


    // get last part of absolute path (example home/dir1/dir2/file.hx -> file.hx)
    std::string file_path_stripped = file_path.substr(file_path.find_last_of("/") + 1, file_path.size());


    // strip file extension (example file.hx -> file)
    file_path_stripped = file_path_stripped.substr(0, file_path_stripped.size() - 3);

    std::string text = load_hx_file(file_path);

    lexer lex = lexer(text);
    std::vector<token> tokens = lex.tokenize();
    
    for (const auto& tok : tokens)
    {
        std::println("{}", get_string_from_token_type(tok.type));
    }

    parser parsr = parser(std::make_unique<lexer>(lex));
    uptr<program> prog = parsr.parse_program();
    
    debug_visitor d_visitor;
    d_visitor.visit_program(prog);
    instruction_generator instruction_gen;
    instruction_gen.visit_program(prog);
    
    /*
    system(string_format("nasm -f elf64 %s.asm -o %s.o",
                file_path_stripped.c_str(), file_path_stripped.c_str()).c_str());
    
    system(string_format("gcc -no-pie %s.o -o %s", file_path_stripped.c_str(), file_path_stripped.c_str()).c_str());
    
    */
}
}


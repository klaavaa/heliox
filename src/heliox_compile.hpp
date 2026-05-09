#pragma once

#include <string>
#include <print>

#include "heliox_lexer.hpp"
#include "heliox_parser.hpp"
#include "heliox_error.hpp"
#include "heliox_file.hpp"

#include "heliox_symbol_visitor.hpp"
#include "heliox_instruction_gen.hpp"
#include "heliox_codegen.hpp"



namespace hx  
{
inline void compile(const std::vector<std::string>& file_paths, const std::string& output_path)
{
    std::vector<uptr<Program>> programs;
    std::vector<sptr<SymbolTable>> tables;
    std::vector<std::string> asm_file_paths;
    for (const auto& file_path : file_paths)
    {
    if (file_path.substr(file_path.size() - 4) != ".hlx")
    {
        hx::Error error;
        error.error_type = HX_NOT_HELIOX_FILE;
        error.line = 0;
        error.info = "Not a heliox file (.hlx)";
        hx::Logger::log_error(error);
        exit(1);
    }

    // get last part of absolute path (example home/dir1/dir2/file.hlx -> file.hlx)
    std::string file_path_stripped = file_path.substr(file_path.find_last_of("/") + 1, file_path.size());

    // strip file extension (example file.hlx -> file)
    file_path_stripped = file_path_stripped.substr(0, file_path_stripped.size() - 4);
    asm_file_paths.emplace_back(file_path_stripped);

    std::string text = load_hx_file(file_path);

    Lexer lexer = Lexer(text);
    std::vector<Token> tokens = lexer.tokenize();

    /* 
    for (const auto& tok : tokens)
    {
        std::println("{}", get_string_from_token_type(tok.type));
    } */

    Parser parser = Parser(std::make_unique<Lexer>(lexer));
    uptr<Program> program = parser.parse_program();
    
    
    
    sptr<SymbolTable> global_table = std::make_shared<SymbolTable>();
    
    SymbolVisitor sv(global_table);
    sv.visit_program(program);
    programs.push_back(std::move(program));
    tables.push_back(global_table);
    }  
    

    for (size_t i = 0; i < programs.size(); i++)
    {
        for (const auto& import : programs[i]->imports)
        {
            for (size_t j = 0; j < tables.size(); j++)
            {
                if (i == j) continue;
                tables[i]->import_module(tables[j], import, programs[i]);
            }
        }
    }

    for (size_t i = 0; i < programs.size(); i++)
    {
        auto& program = programs[i];
        auto& global_table = tables[i];
    InstructionGenerator instruction_gen(global_table);
    instruction_gen.visit_program(program);
    
    LinearScanRegisterAllocation linear_scan(instruction_gen.instruction_data, global_table);    
    linear_scan.scan();


    CodeGeneration codegen(global_table, linear_scan.function_location_data);
    std::string generated_nasm = codegen.generate(instruction_gen.instruction_data);
    std::println("{}", generated_nasm);
    create_assembly_file(asm_file_paths[i], generated_nasm);
    }
    /*
    system(string_format("nasm -f elf64 %s.asm -o %s.o",
                file_path_stripped.c_str(), file_path_stripped.c_str()).c_str());
    
    system(string_format("gcc -no-pie %s.o -o %s", file_path_stripped.c_str(), file_path_stripped.c_str()).c_str());
    
    */
}
}


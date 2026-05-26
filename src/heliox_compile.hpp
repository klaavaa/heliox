#pragma once

#include <string>
#include <print>

#include "heliox_lexer.hpp"
#include "heliox_parser.hpp"
#include "heliox_error.hpp"
#include "heliox_file.hpp"

#include "heliox_symbol_table.hpp"
//#include "heliox_symbol_visitor.hpp"
//#include "heliox_instruction_gen.hpp"
//#include "heliox_codegen.hpp"



namespace hx  
{
inline void compile(const std::vector<std::string>& file_paths, const std::string& output_path)
{
    std::vector<uptr<Program>> programs;
    std::vector<std::string> asm_file_paths;

    std::vector<uptr<TranslationUnit>> translation_units;
    
    for (const auto& file_path : file_paths)
    {
    if (file_path.substr(file_path.size() - 4) != ".hlx")
    {
        Logger::error(file_path, HX_NOT_HELIOX_FILE, "File is not a .hlx file");
    }

    // get last part of absolute path (example home/dir1/dir2/file.hlx -> file.hlx)
    std::string file_path_stripped = file_path.substr(file_path.find_last_of("/") + 1, file_path.size());

    // strip file extension (example file.hlx -> file)
    file_path_stripped = file_path_stripped.substr(0, file_path_stripped.size() - 4);
    asm_file_paths.emplace_back(file_path_stripped);

    std::string text = load_hx_file(file_path);

    //Lexer lexer = Lexer(text, file_path);
    //std::vector<Token> tokens = lexer.tokenize();
    /* 
    for (const auto& tok : tokens)
    {
        std::println("{}", get_string_from_token_type(tok.type));
    } */

    Parser parser = Parser(std::make_unique<Lexer>(text, file_path));

    uptr<TranslationUnit> tu = parser.parse_translation_unit(); 
    translation_units.push_back(std::move(tu));
    }  
    
    // Creates a program which contains all modules
    uptr<Program> program = std::make_unique<Program>(translation_units);


    for (auto& tu : translation_units)
    {
        // create a symbol table for each translation unit
        // the imports are handled here by fetching imported symbols from program's global module
        auto global_table = create_global_table_for_translation_unit(tu, program);

        
    }

    

    /*for (size_t i = 0; i < programs.size(); i++)
    {
        for (const auto& import : programs[i]->imports)
        {
            for (size_t j = 0; j < tables.size(); j++)
            {
                if (i == j) continue;
                tables[i]->import_module(tables[j], import, programs[i]);
            }
        }
    }*/
    /*
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


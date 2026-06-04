#pragma once

#include <string>
#include <print>

#include "heliox_compile_flags.hpp"

#include "heliox_lexer.hpp"
#include "heliox_parser.hpp"
#include "heliox_error.hpp"
#include "heliox_file.hpp"

#include "heliox_symbol_table.hpp"
#include "heliox_instruction_gen.hpp"
#include "heliox_liveness_analysis.hpp"
#include "heliox_register_allocation.hpp"
#include "heliox_code_generation.hpp"



namespace hx  
{
inline void compile(const std::vector<std::string>& file_paths, const std::string& output_path)
{
    const CompileFlags& flags = get_compile_flags();
    
    if (!flags.output_file.empty() && (flags.compile_only || flags.compile_and_assemble_only) && (file_paths.size() > 1))
    {
        Logger::pre_compile_error("Output file cannot be specified with -c or -S flags with multiple files");
    }

    std::vector<uptr<Program>> programs;
    std::vector<std::string> stripped_file_paths;
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
    stripped_file_paths.emplace_back(file_path_stripped);

    std::string text = load_hx_file(file_path);

    //Lexer lexer = Lexer(text, file_path);
    //std::vector<Token> tokens = lexer.tokenize();
    /* 
    for (const auto& tok : tokens)
    {
        std::println("{}", get_string_from_token_type(tok.type));
    } */
    Lexer lexer(text, file_path);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser = Parser(tokens);

    uptr<TranslationUnit> tu = parser.parse_translation_unit(); 
    translation_units.push_back(std::move(tu));
    }  
    
    // Creates a program which contains all modules
    uptr<Program> program = std::make_unique<Program>(translation_units);
    size_t i = 0;


    for (auto& tu : translation_units)
    {
        //Logger::info("generating IR for '{}'", tu->filename);
        // create a symbol table for each translation unit
        // the imports are handled here by fetching imported symbols from program's global module
        auto global_table = create_global_table_for_translation_unit(tu, program);

        //print_table(global_table);
        // generate IR instructions
        InstructionGenerator instruction_gen(std::move(tu), global_table);
        
        IRUnit ir_unit = instruction_gen.generate_instructions();
        //print_ir_unit(ir_unit);

        // generate live-ranges for virtual registers
        perform_liveness_analysis_on_unit(ir_unit, global_table);
        //print_live_ranges(ir_unit);
        // preallocate certain registers / stack

        // perform register allocation
        RegisterAllocator register_allocator(ir_unit);
        register_allocator.allocate_registers();
        //print_ir_unit(ir_unit);

        // generate code
        CodeGenerator code_generator(ir_unit);
        std::string generated_nasm = code_generator.generate();

        std::string asm_output_file = stripped_file_paths[i] + ".asm";
        if (flags.compile_only && !flags.output_file.empty())
        {
            asm_output_file = flags.output_file;
        }
        create_assembly_file(asm_output_file, generated_nasm);
        i++;
    }

    if (flags.compile_only)
    {
        return;
    }

    std::vector<std::string> object_file_paths;

    for (const auto& file_path : stripped_file_paths)
    {
        std::string output_path = file_path + ".o";
        if (flags.compile_and_assemble_only && !flags.output_file.empty())
        {
            output_path = flags.output_file;
        }
        object_file_paths.push_back(output_path);

        #ifdef _WIN32
        std::system(std::format("nasm -fwin64 {}.asm -o {}", file_path, output_path).c_str());
        #else
        std::system(std::format("nasm -felf64 {}.asm -o {}", file_path, output_path).c_str());
        #endif

    }

    if (flags.compile_and_assemble_only)
    {
        return;
    }

    std::string object_files;
    for (const auto& object_file_path : object_file_paths)
    {
        object_files += object_file_path + " ";
    }

#ifdef _WIN32
        std::string output_executable = "a.exe";
#else
        std::string output_executable = "a.out";
#endif
    if (!flags.output_file.empty())
    {
        output_executable = flags.output_file;
    }

#ifdef _WIN32
    std::system(std::format("gcc {} -o {}", object_files, output_executable).c_str());
#else
    std::system(std::format("gcc -no-pie {} -o {}", object_files, output_executable).c_str());
#endif
}
}


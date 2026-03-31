#pragma once

#include <string>
#include <print>

#include "heliox_lexer.hpp"
#include "heliox_parser.hpp"
#include "heliox_error.hpp"
#include "heliox_file.hpp"

#include "heliox_debug_visitor.hpp"
#include "heliox_instruction_gen.hpp"
#include "heliox_codegen.hpp"



namespace hx  
{
inline void compile(const std::string& file_path, const std::string& output_path)
{
    
    if (file_path.substr(file_path.size() - 3) != ".hx")
    {
        hx::Error error;
        error.error_type = HX_NOT_HELIOX_FILE;
        error.line = 0;
        error.info = "Not a heliox file (.hx)";
        hx::Logger::log_error(error);
        exit(1);
    }


    // get last part of absolute path (example home/dir1/dir2/file.hx -> file.hx)
    std::string file_path_stripped = file_path.substr(file_path.find_last_of("/") + 1, file_path.size());


    // strip file extension (example file.hx -> file)
    file_path_stripped = file_path_stripped.substr(0, file_path_stripped.size() - 3);

    std::string text = load_hx_file(file_path);

    Lexer lexer = Lexer(text);
    std::vector<Token> tokens = lexer.tokenize();
    
    for (const auto& tok : tokens)
    {
        std::println("{}", get_string_from_token_type(tok.type));
    }

    Parser parser = Parser(std::make_unique<Lexer>(lexer));
    uptr<Program> program = parser.parse_program();
    
    debug_visitor d_visitor;
    d_visitor.visit_program(program);
    InstructionGenerator instruction_gen;
    instruction_gen.visit_program(program);
    
    LinearScanRegisterAllocation linear_scan(instruction_gen.instruction_data);    
    linear_scan.scan();

    for (auto& live_range : instruction_gen.instruction_data.live_ranges)
    {
        const VirtualRegisterLocation& loc = linear_scan.virtual_register_locations[live_range.reg];
        if (loc.is_spilled) std::println("{} spilled at {}", loc.live_range.reg, loc.stack_position);
        else std::println("{} using register {}", loc.live_range.reg, static_cast<int>(loc.allocated_register));
    }

    uptr<SymbolTable> symbol_table;
    CodeGeneration codegen(std::move(symbol_table));
    /*
    system(string_format("nasm -f elf64 %s.asm -o %s.o",
                file_path_stripped.c_str(), file_path_stripped.c_str()).c_str());
    
    system(string_format("gcc -no-pie %s.o -o %s", file_path_stripped.c_str(), file_path_stripped.c_str()).c_str());
    
    */
}
}


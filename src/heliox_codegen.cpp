#include "heliox_codegen.hpp"
#include <format>

namespace hx
{
    
    codegen::codegen(uptr<symbol_table> global_table)
        : global_table(std::move(global_table))
    {

    }
    
    void codegen::generate(std::vector<instruction_function>& function_instructions)
    {
        
        for (auto& func : function_instructions)
        {
            generate_instruction_function(func); 
        }
    }

    void codegen::generate_instruction_function(instruction_function& instruc_func)
    {
        std::string body = std::format("global {}\n{}:\n", instruc_func.name, instruc_func.name);

        for (auto& triplet : instruc_func.instruction_triplets)
        {
            generate_instruction_triplet(triplet);
        }
        text_section += body;
    }
    
    void codegen::generate_instruction_triplet(instruction_triplet& triplet)
    {
        switch (triplet.instruc)
        {
        case instruction::LOAD_STRING:

        default:
           //TODO ERROR
            std::println("Instruction not implemented");
            exit(-1);

        }

    }
}

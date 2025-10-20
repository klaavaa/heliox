#include "heliox_codegen.hpp"
#include <format>

namespace hx
{
    
    codegen::codegen(uptr<symbol_table> global_table)
        : global_table(global_table)
    {

    }
    
    void codegen::generate(std::queue<instruction_function>& function_instructions)
    {
        

        while (!function_instructions.empty())
        {
            instruction_function& instruc_func = function_instructions.front(); 
            generate_instruction_function(instruc_func); 
            function_instructions.pop();
        }
    }

    void codegen::generate_instruction_function(instruction_function& instruc_func)
    {
        std::string body = std::format("global {}\n{}:\n", instruc_func.name, instruc_func.name);

        while (!instruc_func.triplet_queue.empty())
        {
            instruction_triplet& triplet = instruc_func.triplet_queue.front();
            generate_instruction_triplet(triplet);
            instruc_func.triplet_queue.pop();
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

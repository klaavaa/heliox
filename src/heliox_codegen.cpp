#include "heliox_codegen.hpp"
#include <format>

namespace hx
{
    
    CodeGeneration::CodeGeneration(uptr<SymbolTable> global_table)
        : global_table(std::move(global_table))
    {

    }
    
    void CodeGeneration::generate(std::vector<InstructionFunction>& function_instructions)
    {
        for (auto& func : function_instructions)
        {
            emit_instruction_function(func); 
        }
    }

    void CodeGeneration::emit_instruction_function(InstructionFunction& instruc_func)
    {
        std::string body = std::format("global {}\n{}:\n", instruc_func.name, instruc_func.name);

        for (auto& triplet : instruc_func.instruction_triplets)
        {
            emit_instruction_triplet(triplet);
        }
        text_section += body;
    }
    
    void CodeGeneration::emit_instruction_triplet(InstructionTriplet& triplet)
    {
        
        switch (triplet.instruc)
        {
    
        case Instruction::LOAD_STRING:
            
        default:
           //TODO ERROR
            std::println("Instruction not implemented");
            exit(-1);

        }

    }
}

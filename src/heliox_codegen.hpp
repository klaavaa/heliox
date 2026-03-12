#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
namespace hx
{
    
    struct register_allocation
    {
        Register reg;
        virtual_register vr;

        int32_t stack_spill;
    };


    class CodeGeneration
    {
    public:
        CodeGeneration(uptr<SymbolTable> global_table);
        void generate(std::vector<InstructionFunction>& function_instructions);
    private:
        void emit_instruction_function(InstructionFunction& instruc_func); 
        void emit_instruction_triplet(InstructionTriplet& triplet); 
         
         

    private:
        
        std::string externs;
        std::string data_section;
        std::string text_section;

        uptr<SymbolTable> global_table; 
    };
}

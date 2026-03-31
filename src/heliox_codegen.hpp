#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_linearscan.hpp"

namespace hx
{

    class CodeGeneration
    {
    public:
        CodeGeneration(uptr<SymbolTable> global_table);
        void generate(InstructionData& instruction_data);
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

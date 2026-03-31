#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include <bitset>

namespace hx
{
    // Register rax -> r18
    // spill r18
    // Register rax -> r23
    // 
    struct Location
    {
        Register reg = Register::NOREG;
        int32_t stack_spill;
    };


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

        std::bitset<16> used_registers;
        std::array<virtual_register, 16> register_contents;
        std::unordered_map<virtual_register, Location> virtual_locations;
    };
}

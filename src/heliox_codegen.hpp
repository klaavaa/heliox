#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_linearscan.hpp"

namespace hx
{

    class CodeGeneration
    {
    public:
        CodeGeneration(sptr<SymbolTable> global_table, sptr<std::unordered_map<std::string, std::unordered_map<virtual_register, VirtualRegisterLocation>>> all_vr_locations);
        std::string generate(InstructionData& instruction_data);
    private:

        void generate_data_section();

        std::string emit_instruction_function(InstructionFunction& instruc_func); 
        std::string emit_instruction_triplet(InstructionTriplet& triplet); 
        
        std::string get_location(virtual_register vr);
        std::string get_location(Item item);
         

    private:
        
        std::string externs;
        std::string data_section;
        std::string text_section;
        std::string bss_section;

        sptr<SymbolTable> global_table; 
        sptr<std::unordered_map<std::string, VirtualRegisterLocationMap>> all_vr_locations;
        VirtualRegisterLocationMap current_func_vr_locations;

    };
}

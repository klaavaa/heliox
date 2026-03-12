#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
namespace hx
{
    
    struct register_allocation
    {
        registers reg;
        virtual_register vr;

        int32_t stack_spill;
    };


    class codegen
    {
    public:
        codegen(uptr<symbol_table> global_table);
        void generate(std::vector<instruction_function>& function_instructions);
    private:
        void emit_instruction_function(instruction_function& instruc_func); 
        void emit_instruction_triplet(instruction_triplet& triplet); 
         
         

    private:
        
        std::string externs;
        std::string data_section;
        std::string text_section;

        uptr<symbol_table> global_table; 

    };
}

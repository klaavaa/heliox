#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_linearscan.hpp"

namespace hx
{

    class CodeGeneration
    {
    public:
        CodeGeneration(sptr<SymbolTable> global_table, sptr<FunctionDataInfoMap> function_data_info_map);
        std::string generate(InstructionData& instruction_data);
    private:

        void generate_data_section();

        std::string emit_instruction_function(InstructionFunction& instruc_func); 
        std::string emit_instruction_triplet(InstructionTriplet& triplet); 
        
        std::string get_location(virtual_register vr);
        std::string get_location(virtual_register vr, RegisterSize reg_size);
        std::string get_location(Item item);
        
        RegisterBitSet get_reserved_registers_at(int64_t position);
        RegisterBitSet get_function_used_registers();

        std::string parse_string(const std::string& str);

    private:
        std::string externs;
        std::string data_section;
        std::string text_section;
        std::string bss_section;

        sptr<SymbolTable> global_table; 
        sptr<FunctionDataInfoMap> function_data_info_map;
        VirtualRegisterLocationMap current_func_vr_locations;
        
        RegisterBitSet callee_saved_registers;
        RegisterBitSet caller_saved_registers;

        bool added_padding_from_caller_save = false;
        bool added_padding_from_callee_save = false;

        std::vector<Register> caller_preserved_registers;
        std::vector<Register> callee_preserved_registers;
        
        int64_t param_stack_position = 0;
    };
}

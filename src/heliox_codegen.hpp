#pragma once
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"
#include <queue>

namespace hx
{
    enum class registers: int
    {
        A=0, B, C, D, SP, BP, SI, DI, R8, R9, R10, R11, R12, R13, R14, R15
    };
    enum class bit64_registers: int
    {
        RAX=0, RBX, RCX, RDX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
    };
    enum class bit32_registers: int
    {
        EAX=0, EBX, ECX, EDX, ESP, EBP, ESI, EDI, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D
    };
    enum class bit16_registers: int
    {
        AX=0, BX, CX, DX, SP, BP, SI, DI, R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W
    };
    enum class bit8_registers: int
    {
        AL=0, BL, CL, DL, SPL, BPL, SIL, DIL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B
    };


    class codegen
    {
    public:
        codegen(uptr<symbol_table> global_table);
        void generate(std::queue<instruction_function>& function_instructions);
    private:
        void generate_instruction_function(instruction_function& instruc_func); 
        void generate_instruction_triplet(instruction_triplet& triplet); 
         

    private:
        
        std::string externs;
        std::string data_section;
        std::string text_section;

        uptr<symbol_table> global_table; 
        uint32_t instruction_count;
    };
}

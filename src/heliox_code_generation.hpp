#pragma once
#include "heliox_instructions.hpp"
#include "heliox_pointer.hpp"

namespace hx
{

class CodeGenerator
{
public:
    CodeGenerator(IRUnit& ir_unit) : ir_unit(ir_unit) {}
    std::string generate();

private:
    std::string get_vr_location(int64_t vr);
    std::string get_location(const IROperand operand);
    void emit(const std::string_view asm_instruction, const IROperand dst, const IROperand src);
    void emit(const std::string_view asm_instruction, const IROperand src);
    void emit(const std::string_view asm_instruction);
    void emit(const std::string_view asm_instruction, const std::string_view src);
    void emit(const std::string_view asm_instruction, const std::string_view dst, const std::string_view src);
    void emit_function(IRFunction& ir_function);
    void emit_instruction(IRInstruction& instruction);
    void emit_load_immediate(IRInstruction& load);
    void emit_mov();
    void emit_add();

    IRUnit& ir_unit;
    IRFunction* current_function;

    std::string extern_section;
    std::string data_section;
    std::string bss_section;
    std::string text_section;
};


} // namespace hx
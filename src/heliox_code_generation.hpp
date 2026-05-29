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
    std::string get_vr_location(int64_t vr, uint32_t byte_size);
    std::string get_location(const IROperand operand, uint32_t byte_size);
    std::string get_location(const IROperand operand);
    void emit(const std::string_view asm_instruction, const IROperand dst, const IROperand src);
    void emit(const std::string_view asm_instruction, const IROperand src);
    void emit(const std::string_view asm_instruction);
    void emit(const std::string_view asm_instruction, const std::string_view src);
    void emit(const std::string_view asm_instruction, const std::string_view dst, const std::string_view src);
    void emit_function(IRFunction& ir_function);
    void emit_instruction(IRInstruction& instruction);

    void emit_data_section();

    int64_t allignment_to_add_before_call();

    void save_callee_preserved_registers();
    void load_callee_preserved_registers();

    IRUnit& ir_unit;
    IRFunction* current_function;

    std::string extern_section;
    std::string data_section;
    std::string bss_section;
    std::string text_section;

    int64_t arg_push_count = 0;
    bool aligned_before_call = false;
    std::set<Register> registers_to_preserve;

};


} // namespace hx
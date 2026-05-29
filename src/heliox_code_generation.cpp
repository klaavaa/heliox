#include "heliox_code_generation.hpp"

namespace hx
{

std::string CodeGenerator::generate()
{

    emit_data_section();

    data_section += "section .text\n";
    for (auto& ir_function : ir_unit.ir_functions)
    {
        current_function = &ir_function;
        emit_function(ir_function);
    }

    return extern_section + data_section  + bss_section + text_section;  
}

void CodeGenerator::emit_function(IRFunction& ir_function)
{
    if (ir_function.is_extern)
    {
        extern_section += std::format("extern {}\n", ir_function.name);
        return;
    }

    text_section += std::format("global {}\n{}:\n", ir_function.name, ir_function.name);

    emit("push", "rbp");
    emit("mov", "rbp", "rsp");

    if (ir_function.total_stack_allocated != 0)
    {
        emit("sub", "rsp", std::to_string(ir_function.total_stack_allocated));
    }

    for (auto& instruction : ir_function.instructions)
    {
        emit_instruction(instruction);
    }

}

void CodeGenerator::emit_instruction(IRInstruction& instruction)
{

    switch (instruction.type)
    {
    case IRInstructionType::LOAD_IMMEDIATE:
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::LOAD_MEM_INDEX:
        emit("lea", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::MOV:
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::ARG_PUSH:
        arg_push_count += 1;
        //stack alignment
        if (instruction.src2.kind != IROperandKind::NONE)
            emit("sub", "rsp", "8");
        // set the reg_size manually because the vr reg size may not be 8  
        emit("push", get_location(instruction.src1, 8));
        return;
    case IRInstructionType::IADD:
        emit("add", instruction.src1, instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::ISUB:
        emit("sub", instruction.src1, instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::IDIV:
        emit("xor", "rdx", "rdx");
        emit("cqo");
        emit("idiv", instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::IMUL:
        emit("imul", instruction.src1, instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;

    case IRInstructionType::RETURN:
        emit("mov", instruction.dst, instruction.src1);
        emit("mov", "rsp", "rbp");
        emit("pop", "rbp");
        emit("ret");
        return;

    case IRInstructionType::FUNCTION_CALL:
        {
            // TODO FIX THIS SHIIIIIT
        int64_t to_add = allignment_to_add_before_call(); 
        if (to_add)
        {
            emit("sub", "rsp", std::to_string(to_add));
        }
    #ifdef _WIN32
        emit("sub", "rsp", "32");
        emit("call", instruction.src1);
        emit("add", "rsp", "32");
    #else
        emit("call", instruction.src1);
    #endif
        if (arg_push_count > 0)
        {
            emit("add", "rsp", std::format("{}", 8 * arg_push_count));
            arg_push_count = 0;
        }

        return;
        }

    case IRInstructionType::MOV_ARG:
        emit("mov", instruction.dst, instruction.src1);
        return;
    }


}
int64_t CodeGenerator::allignment_to_add_before_call()
{
    int64_t total = current_function->total_stack_allocated + 8*arg_push_count;
    return total - ((total + 15) & ~15);
}
std::string CodeGenerator::get_vr_location(int64_t vr)
{
    uint32_t byte_size = current_function->virtual_register_types.at(vr).byte_size;
    return get_vr_location(vr, byte_size);
}

std::string CodeGenerator::get_vr_location(int64_t vr, uint32_t byte_size)
{
    Location& location = current_function->virtual_register_locations.at(vr);
    if (location.kind == LocationKind::REGISTER)
    {
        return get_register(location.reg, byte_size);
    }
    else // STACK
    {
        switch (byte_size)
        {
        case 8:
            return std::format("qword[rbp - {}]", location.stack);
        case 4:
            return std::format("dword[rbp - {}]", location.stack);
        case 2:
            return std::format("word[rbp - {}]", location.stack);
        case 1:
            return std::format("byte[rbp - {}]", location.stack);
        default:
            Logger::error("", HX_ILLEGAL_REG_SIZE, std::format("tried to get a location of size: {}", byte_size));
        }
    }
}

std::string CodeGenerator::get_location(const IROperand operand)
{
    uint32_t instruction_size = current_function->virtual_register_types.at(operand.value).byte_size;
    return get_location(operand, instruction_size);
}

std::string CodeGenerator::get_location(const IROperand operand, uint32_t byte_size)
{
    switch (operand.kind)
    {
    case IROperandKind::VIRTUAL_REGISTER:
        return get_vr_location(operand.value, byte_size);
    case IROperandKind::LITERAL_LOCATION:
        if (ir_unit.allocated_literals.at(operand.value).type == LiteralType::FUNCTION_NAME)
        {
            return std::format("{}", ir_unit.allocated_literals.at(operand.value).value);
        }
        return std::format("[rel $L{}]", operand.value);
    case IROperandKind::IMMEDIATE_VALUE:
        return std::format("{}", operand.value);
    default:
        Logger::error("", HX_ILLEGAL_LOCATION, "Trying to get an illegal location");
    }

}

void CodeGenerator::emit_data_section()
{
    data_section += "section .data\n";
    for (auto& [key, literal] : ir_unit.allocated_literals)
    {
        if (literal.type == LiteralType::STRING)
        {
            data_section += std::format("\t$L{} db {}\n", key, literal.value);
        }
    }
}

void CodeGenerator::emit(const std::string_view asm_instruction, const IROperand dst, const IROperand src)
{
    uint32_t instruction_size = current_function->virtual_register_types.at(dst.value).byte_size;
    text_section += std::format("\t{} {}, {}\n", asm_instruction, get_location(dst, instruction_size), get_location(src, instruction_size));
}
void CodeGenerator::emit(const std::string_view asm_instruction, const IROperand src)
{
    text_section += std::format("\t{} {}\n", asm_instruction, get_location(src));
}
void CodeGenerator::emit(const std::string_view asm_instruction)
{
    text_section += std::format("\t{}\n", asm_instruction);
}
void CodeGenerator::emit(const std::string_view asm_instruction, const std::string_view src)
{
    text_section += std::format("\t{} {}\n", asm_instruction, src);
}

void CodeGenerator::emit(const std::string_view asm_instruction, const std::string_view dst, const std::string_view src)
{
    text_section += std::format("\t{} {}, {}\n", asm_instruction, dst, src);
}

} // namespace hx
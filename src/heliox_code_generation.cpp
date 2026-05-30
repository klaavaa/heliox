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


    save_callee_preserved_registers();

    emit("push", "rbp");
    emit("mov", "rbp", "rsp");

    if (ir_function.total_stack_allocated != 0)
    {
        // fix the alignment based on the preserved registers (by default the total_stack_allocated is aligned to 16 bytes, so if an odd number of pushes occur, we need to add 8)
        // + 1 => push rbp
        ir_function.total_stack_allocated += ((registers_to_preserve.size() + 1) % 2) * 8;
        emit("sub", "rsp", std::to_string(ir_function.total_stack_allocated));
    }


    for (auto& instruction : ir_function.instructions)
    {
        emit_instruction(instruction);
    }

    registers_to_preserve.clear();
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
    case IRInstructionType::STORE_MEM:
        emit_mem_write("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::DEREF:
        emit_mem_read("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::REGISTER_ARG:
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::ARG_PUSH:
        arg_push_count += 1;

        if (instruction.src2.kind != IROperandKind::NONE)
        {
            aligned_before_call = true;
            emit("sub", "rsp", "8");
        }

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

    case IRInstructionType::IMOD:
        emit("xor", "rdx", "rdx");
        emit("cqo");
        emit("idiv", instruction.src2);
        emit("mov", get_location(instruction.dst), get_register(Register::D, get_vr_type(instruction.dst).byte_size));
        return;
    case IRInstructionType::BITWISE_AND:
        emit("and", instruction.src1, instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::BITWISE_OR:
        emit("or", instruction.src1, instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::BITWISE_XOR:
        emit("xor", instruction.src1, instruction.src2);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::BITWISE_NOT:
        emit("not", instruction.src1);
        emit("mov", instruction.dst, instruction.src1);
        return;
    case IRInstructionType::RETURN:
        emit("mov", instruction.dst, instruction.src1);
        emit("mov", "rsp", "rbp");
        emit("pop", "rbp");
        load_callee_preserved_registers();
        emit("ret");
        return;

    case IRInstructionType::FUNCTION_CALL:
        {
    #ifdef _WIN32
        emit("sub", "rsp", "32");
        emit("call", instruction.src1);
        emit("add", "rsp", "32");
    #else
        emit("call", instruction.src1);
    #endif

        int64_t to_add = 8*arg_push_count + (int64_t)aligned_before_call * 8; 
        if (to_add)
        {
            emit("add", "rsp", std::to_string(to_add));
        }
        arg_push_count = 0;
        aligned_before_call= false;

        return;
        }

    case IRInstructionType::MOV_ARG:
        emit("mov", instruction.dst, instruction.src1);
        return;

    case IRInstructionType::CMP_EQU:
        emit("cmp", instruction.src1, instruction.src2);
        emit("sete", instruction.dst);
        return;

    case IRInstructionType::CMP_NEQU:
        emit("cmp", instruction.src1, instruction.src2);
        emit("setne", instruction.dst);
        return;

    case IRInstructionType::CMP_GT:
        emit("cmp", instruction.src1, instruction.src2);
        if (is_unsigned(get_vr_type(instruction.src1)))
            emit("seta", instruction.dst);
        else
            emit("setg", instruction.dst);
        return;

    case IRInstructionType::CMP_LT:
        emit("cmp", instruction.src1, instruction.src2);
        if (is_unsigned(get_vr_type(instruction.src1)))
            emit("setb", instruction.dst);
        else
            emit("setl", instruction.dst);
        return;

    case IRInstructionType::CMP_GTE:
        emit("cmp", instruction.src1, instruction.src2);
        if (is_unsigned(get_vr_type(instruction.src1)))
            emit("setae", instruction.dst);
        else
            emit("setge", instruction.dst);
        return;

    case IRInstructionType::CMP_LTE:
        emit("cmp", instruction.src1, instruction.src2);
        if (is_unsigned(get_vr_type(instruction.src1)))
            emit("setbe", instruction.dst);
        else
            emit("setle", instruction.dst);
        return;

    case IRInstructionType::JMP:
        emit("jmp", instruction.src2);
        return;
    case IRInstructionType::JMP_IF:
        emit("test", instruction.src1, instruction.src1);
        emit("jnz", instruction.src2);
        return;
    case IRInstructionType::JMP_IF_NOT:
        emit("test", instruction.src1, instruction.src1);
        emit("jz", instruction.src2);
        return;
    case IRInstructionType::LABEL:
        emit_label(instruction.src2);
        return;
    default:
        Logger::not_implemented();
    }


}


void CodeGenerator::save_callee_preserved_registers()
{
    for (const auto& [vr, loc] : current_function->virtual_register_locations)
    {
        if (loc.kind != LocationKind::REGISTER) continue;
        if (!g_register_data.callee_saved_registers.contains(loc.reg)) continue;
        if (registers_to_preserve.contains(loc.reg)) continue;
        registers_to_preserve.insert(loc.reg);
        emit("push", get_register(loc.reg, 8));
    }
}

int64_t CodeGenerator::allignment_to_add_before_call()
{
    return ((arg_push_count + registers_to_preserve.size()) % 2) * 8;
}

void CodeGenerator::load_callee_preserved_registers()
{
    for (auto it = registers_to_preserve.rbegin(); it != registers_to_preserve.rend(); it++)
    {
        const Register r = *it;
        emit("pop", get_register(r, 8));
    }
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
        int64_t stack_pos = abs(location.stack);
        char op = '-';

        if (location.stack < 0)
        {
            // fix the offset added by preserved registers
            stack_pos += registers_to_preserve.size() * 8;
            op = '+';
        }


        switch (byte_size)
        {
        case 8:
            return std::format("qword[rbp {} {}]", op, stack_pos);
        case 4:
            return std::format("dword[rbp {} {}]", op, stack_pos);
        case 2:
            return std::format("word[rbp  {} {}]", op, stack_pos);
        case 1:
            return std::format("byte[rbp  {} {}]", op, stack_pos);
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
    case IROperandKind::LABEL:
        return std::format(".LB{}", operand.value);
    default:
        Logger::error("", HX_ILLEGAL_LOCATION, std::format("Trying to get an illegal location {}", (int)operand.kind));
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

void CodeGenerator::emit_mem_write(const std::string_view asm_instruction, const IROperand dst, const IROperand src)
{
    uint32_t instruction_size = get_vr_type(dst).byte_size;
    text_section += std::format("\t{} [{}], {}\n", asm_instruction, get_location(dst, 8), get_location(src, instruction_size));
}
void CodeGenerator::emit_mem_read(const std::string_view asm_instruction, const IROperand dst, const IROperand src)
{
    uint32_t instruction_size = get_vr_type(dst).byte_size;

    text_section += std::format("\t{} {}, [{}]\n", asm_instruction, get_location(dst, instruction_size), get_location(src, 8));
}
void CodeGenerator::emit(const std::string_view asm_instruction, const IROperand dst, const IROperand src)
{
    uint32_t instruction_size = get_vr_type(dst).byte_size;
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

void CodeGenerator::emit_jmp(const IROperand label)
{
    text_section += std::format("\tjmp {}\n", get_location(label, 0));
}

void CodeGenerator::emit_label(const IROperand label)
{
    text_section += std::format("{}:\n", get_location(label, 0));
}

type_data CodeGenerator::get_vr_type(const IROperand vr)
{
    return current_function->virtual_register_types.at(vr.value);
}
} // namespace hx
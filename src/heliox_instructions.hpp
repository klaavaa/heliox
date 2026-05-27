#pragma once
#include <print>
#include <format>
#include <vector>
#include <unordered_map>
#include <map>
#include "heliox_registerdata.hpp"
#include "heliox_types.hpp"

namespace hx
{

enum class IRInstructionType 
{
    MOV,
    LOAD_IMMEDIATE,
    LOAD_MEM_INDEX,

    STORE_MEM,
    
    // dst = return value, src1 = function_index

    FUNCTION_CALL,
    RETURN,

    // function argument  src1 = arg to push, src2 = arg index
    MOV_ARG,


    IADD,
    ISUB,
    IDIV,
    IMUL,

    // ==================

};

enum class LiteralType
{
    STRING,
    FUNCTION_NAME,
};

struct AllocatedLiteral
{
    LiteralType type;
    std::string value;
};

struct IRInstruction
{
    IRInstruction(IRInstructionType _type, int64_t _dst, int64_t _src1, int64_t _src2)
    : type(_type), dst(_dst), src1(_src1), src2(_src2) {}
    
    IRInstructionType type;
    
    int64_t dst; 
    int64_t src1;
    int64_t src2;
};


struct IRFunction
{
    std::string name;
    std::vector<IRInstruction> instructions{};
};

struct IRUnit
{
    std::vector<IRFunction> ir_functions;
    std::unordered_map<size_t, AllocatedLiteral> allocated_literals;
    std::unordered_map<int64_t, type_data> virtual_register_types;

    size_t allocate_string_literal(const std::string& value)
    {
        size_t id = allocated_literals.size();
        // todo parse string literals for escape sequences 
        allocated_literals.insert({id, AllocatedLiteral{LiteralType::STRING, value}});
        return id;
    }

    size_t allocate_function_name(const std::string& value)
    {
        size_t id = allocated_literals.size();
        allocated_literals.insert({id, AllocatedLiteral{LiteralType::FUNCTION_NAME, value}});
        return id;
    }

};


inline void print_ir_instruction(IRInstruction& ir_instruction, size_t instruction_number)
{
    std::string prefix = std::format("{:3}", instruction_number);
    switch (ir_instruction.type)
    {
        case IRInstructionType::FUNCTION_CALL:
            std::println("{}  CALL       r{}  <- fun[{}]", prefix, ir_instruction.dst, ir_instruction.src1);
            break;

        case IRInstructionType::MOV_ARG:
            std::println("{}  MOV_ARG    r{}  <- r{}, arg[{}]", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::RETURN:
            std::println("{}  RETURN     r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::LOAD_MEM_INDEX:
            std::println("{}  LOAD_STR   r{}  <- idx[{}]", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::LOAD_IMMEDIATE:
            std::println("{}  LOAD_INT   r{}  <- {}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::MOV:
            std::println("{}  MOV        r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::STORE_MEM:
            std::println("{}  STORE_MEM [r{}] <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;

        case IRInstructionType::IADD:
            std::println("{}  IADD       r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;


        default:
            std::println("instruction {} not implemented yet", static_cast<int>(ir_instruction.type));
            break;
    }
}
inline void print_ir_unit(IRUnit& ir_unit)
{

    for (auto& ir_function : ir_unit.ir_functions)
    {
        std::println("{}():", ir_function.name);
        size_t instruction_number = 1;
        for (auto& inst : ir_function.instructions)
        {
            print_ir_instruction(inst, instruction_number++);
        }
    }
}

} // namespace hx

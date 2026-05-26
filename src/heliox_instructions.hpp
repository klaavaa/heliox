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

using virtual_register = size_t;

enum class IRInstructionType 
{
    LABEL,
    JUMP_TRUE,
    JUMP_FALSE,

    LOAD_INT,
    LOAD_FLOAT,
    LOAD_STRING,
    
    FUNCTION_CALL, // dst = return value, src1 = function_index
    RETURN,

    // function argument  src1 = arg to push, src2 = arg index
    PUSH_ARG
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
    IRInstructionType type;
    virtual_register dst; 
    virtual_register src1;
    virtual_register src2;
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
    std::unordered_map<virtual_register, type_data> virtual_register_types;

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
            std::println("{}  CALL      {} {}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::PUSH_ARG:
            std::println("{}  PUSH_ARG  {} {}", prefix, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::RETURN:
            std::println("{}  RETURN    {} {}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::LOAD_STRING:
            std::println("{}  LOAD_STR  {} {}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::LOAD_INT:
            std::println("{}  LOAD_INT  {} {}", prefix, ir_instruction.dst, ir_instruction.src1);
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

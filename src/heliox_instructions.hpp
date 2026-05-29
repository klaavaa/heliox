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

inline std::string parse_string_for_asm(const std::string& str)
{
        std::string parsed_string = "\"";
    bool escaped = false;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] == '\\')
        {
            if (!escaped)
                parsed_string += '"';
            i++;
            escaped = true;

            switch (str[i])
            {
            case 'n':
                parsed_string += ", 10";
                continue;
            case 't':
                parsed_string += ", 9";
                continue;
            case '0':
                parsed_string += ", 0";
                continue;
            default:
                parsed_string += str[i];
                escaped = false;
                continue;
            }
            
        }
        if (escaped)
        {
            parsed_string += ",\"";
            escaped = false;
        }
        parsed_string += str[i];
        
    }
    if (!escaped)
        parsed_string += '"';
    return parsed_string;
}


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
    ARG_PUSH,

    DEREF,

    IADD,
    ISUB,
    IDIV,
    IMUL,

    // ==================

};

enum class LocationKind
{
    REGISTER,
    STACK,
};

struct Location
{
    static Location Reg(Register r) { return {.kind = LocationKind::REGISTER, .reg = r };}
    static Location Stack(int64_t offset) { return {.kind = LocationKind::STACK, .stack = offset };}

    LocationKind kind;
    union {
        Register reg;
        int64_t stack;
    };
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

enum class IROperandKind
{
    NONE,
    VIRTUAL_REGISTER,
    IMMEDIATE_VALUE,
    LITERAL_LOCATION,
    ARG_NUMBER,
};

struct IROperand
{
    IROperand(IROperandKind _kind, int64_t _value) : kind(_kind), value(_value) {}

    static IROperand None()
    {
        return {IROperandKind::NONE, -1};
    }
    static IROperand Vr(int64_t val)
    {
        return {IROperandKind::VIRTUAL_REGISTER, val};
    }
    static IROperand Immediate(int64_t val)
    {
        return {IROperandKind::IMMEDIATE_VALUE, val};
    }
    static IROperand Arg(int64_t val)
    {
        return {IROperandKind::ARG_NUMBER, val};
    }
    static IROperand Literal(int64_t val)
    {
        return {IROperandKind::LITERAL_LOCATION, val};
    }


    IROperandKind kind;
    int64_t value;
};

struct IRInstruction
{
    IRInstruction(IRInstructionType _type, IROperand _dst, IROperand _src1, IROperand _src2)
    : type(_type), dst(_dst), src1(_src1), src2(_src2) {}
    
    IRInstructionType type;

    IROperand dst; 
    IROperand src1;
    IROperand src2;
};

struct LiveRange
{
    LiveRange(size_t _start, size_t _end) : start(_start), end(_end) {}
    size_t start;
    size_t end;
};

struct RegisterReservation
{
    static RegisterReservation Stack() { return RegisterReservation{.on_stack = true};}
    static RegisterReservation Reg(Register reg) { return RegisterReservation{.on_stack = false, .reg = reg};}

    // force the vr to go to stack
    bool on_stack;

    // register that the vr will always map to
    Register reg;
    // other registers that will get reserved also (for example for idiv rdx etc.)
    std::vector<Register> non_vr_regs;

};

struct IRFunction
{
    std::string name;
    bool is_extern;
    std::vector<IRInstruction> instructions{};
    std::unordered_map<int64_t, type_data> virtual_register_types;
    std::map<int64_t, LiveRange> live_ranges;
    std::map<int64_t, Location> virtual_register_locations;

    std::map<int64_t, RegisterReservation> register_reservations;

    int64_t total_stack_allocated = 0;
};

struct IRUnit
{
    std::vector<IRFunction> ir_functions;
    std::unordered_map<size_t, AllocatedLiteral> allocated_literals;

    size_t allocate_string_literal(const std::string& value)
    {
        size_t id = allocated_literals.size();
        // todo parse string literals for escape sequences 
        std::string parsed = parse_string_for_asm(value);
        allocated_literals.insert({id, AllocatedLiteral{LiteralType::STRING, parsed}});
        return id;
    }

    size_t allocate_function_name(const std::string& value)
    {
        size_t id = allocated_literals.size();
        allocated_literals.insert({id, AllocatedLiteral{LiteralType::FUNCTION_NAME, value}});
        return id;
    }

};
} // namespace hx

template<>
struct std::formatter<hx::IROperand> : std::formatter<std::string_view> {
    auto format(const hx::IROperand& op, format_context& ctx) const {
        return std::formatter<std::string_view>::format(
            std::to_string(op.value),
            ctx
        );
    }
};

namespace hx
{
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
            std::println("{}  LOAD_MEM   r{}  <- idx[{}]", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::LOAD_IMMEDIATE:
            std::println("{}  LOAD_INT   r{}  <- {}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::MOV:
            std::println("{}  MOV        r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::ARG_PUSH:
            std::println("{}  ARG_PUSH        <- r{}", prefix, ir_instruction.src1);
            break;
        case IRInstructionType::STORE_MEM:
            std::println("{}  STORE_MEM [r{}] <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;

        case IRInstructionType::IADD:
            std::println("{}  IADD       r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::ISUB:
            std::println("{}  ISUB       r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::IMUL:
            std::println("{}  IMUL       r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::IDIV:
            std::println("{}  IDIV       r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::DEREF:
            std::println("{}  DEREF      r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
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


#pragma once
#include <print>
#include <format>
#include <vector>
#include <unordered_map>
#include <map>
#include <optional>
#include "heliox_registerdata.hpp"
#include "heliox_types.hpp"



namespace hx
{

inline int64_t align_up(int64_t offset, int64_t align)
{
    return (offset + align - 1) & ~(align - 1);
}



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
    LOAD_FLOAT32,
    LOAD_FLOAT64,

    STORE_MEM,
    
    // dst = return value, src1 = function_index

    FUNCTION_CALL,
    RETURN,

    // function argument  src1 = arg to push, src2 = arg index
    MOV_ARG,
    MOV_VARARG,
    ARG_PUSH,
    REGISTER_ARG,

    DEREF,
    ADDR_OF,

    IADD,
    ISUB,
    IDIV,
    IMUL,
    IMOD,

    F64ADD,
    F64SUB,
    F64DIV,
    F64MUL,

    F32ADD,
    F32SUB,
    F32DIV,
    F32MUL,

    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    BITWISE_NOT,

    JMP,
    JMP_IF,
    JMP_IF_NOT,

    ICMP_EQU,
    ICMP_NEQU,
    ICMP_GT,
    ICMP_LT,
    ICMP_GTE,
    ICMP_LTE,

    F32CMP_EQU,
    F32CMP_NEQU,
    F32CMP_GT,
    F32CMP_LT,
    F32CMP_GTE,
    F32CMP_LTE,

    F64CMP_EQU,
    F64CMP_NEQU,
    F64CMP_GT,
    F64CMP_LT,
    F64CMP_GTE,
    F64CMP_LTE,

    LABEL,
    
    CONVERT_F64_TO_F32,
    CONVERT_F32_TO_F64,

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
    FLOAT32,
    FLOAT64,
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
    LABEL
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

    static IROperand Label(int64_t val)
    {
        return {IROperandKind::LABEL, val};
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
    static RegisterReservation Stack() { return RegisterReservation{.must_be_register=false, .on_stack = true, .reg = std::nullopt};}
    static RegisterReservation Reg(Register reg) { return RegisterReservation{.must_be_register = false, .on_stack = false, .reg = reg};}
    static RegisterReservation SomeRegister() { return RegisterReservation{.must_be_register = true, .on_stack = false, .reg = std::nullopt};}

    // force the vr to be some register
    bool must_be_register = false;
    // force the vr to go to stack
    bool on_stack = false;

    // register that the vr will always map to
    std::optional<Register> reg = std::nullopt;
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

    size_t allocate_float64_literal(const std::string& value)
    {
        size_t id = allocated_literals.size();
        double val = std::stod(value);
        std::string parsed_val = std::to_string(*reinterpret_cast<int64_t*>(&val));
        allocated_literals.insert({id, AllocatedLiteral{LiteralType::FLOAT64, parsed_val}});
        return id;
    }
    size_t allocate_float32_literal(const std::string& value)
    {
        size_t id = allocated_literals.size();
        float val = std::stof(value);
        std::string parsed_val = std::to_string(*reinterpret_cast<int64_t*>(&val));
        allocated_literals.insert({id, AllocatedLiteral{LiteralType::FLOAT32, parsed_val}});
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
        case IRInstructionType::MOV_VARARG:
            std::println("{}  MOV_VARARG r{}  <- r{}, arg[{}]", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
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
        case IRInstructionType::LOAD_FLOAT32:
            std::println("{}  LOAD_F32   r{}  <- idx[{}]", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::LOAD_FLOAT64:
            std::println("{}  LOAD_F64   r{}  <- idx[{}]", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::MOV:
            std::println("{}  MOV        r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::ARG_PUSH:
            std::println("{}  ARG_PUSH        <- r{}", prefix, ir_instruction.src1);
            break;

        case IRInstructionType::REGISTER_ARG:
            std::println("{}  RGISTER_ARG r{} <- r{}, arg[{}]", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
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

        case IRInstructionType::F64ADD:
            std::println("{}  F64ADD     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::F64SUB:
            std::println("{}  F64SUB     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::F64MUL:
            std::println("{}  F64MUL     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::F64DIV:
            std::println("{}  F64DIV     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::F32ADD:
            std::println("{}  F32ADD     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::F32SUB:
            std::println("{}  F32SUB     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::F32MUL:
            std::println("{}  F32MUL     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);

            break;
        case IRInstructionType::F32DIV:
            std::println("{}  F32DIV     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::IMOD:
            std::println("{}  IMOD       r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::BITWISE_AND:
            std::println("{}  BIT_AND    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::BITWISE_OR:
            std::println("{}  BIT_OR     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::BITWISE_XOR:
            std::println("{}  BIT_XOR    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::BITWISE_NOT:
            std::println("{}  BIT_NOT    r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;

        case IRInstructionType::DEREF:
            std::println("{}  DEREF      r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::ADDR_OF:
            std::println("{}  ADDR_OF    r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;

        case IRInstructionType::JMP:
            std::println("{}  JMP             <-    , LB{}", prefix, ir_instruction.src2);
            break;
        case IRInstructionType::JMP_IF:
            std::println("{}  JMP_IF          <- r{}, LB{}", prefix, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::JMP_IF_NOT:
            std::println("{}  JMP_NOT         <- r{}, LB{}", prefix, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::LABEL:
            std::println("{}  LABEL           <-    , LB{}", prefix, ir_instruction.src2);
            break;
        case IRInstructionType::ICMP_EQU:
            std::println("{}  ICMP_EQU    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::ICMP_NEQU:
            std::println("{}  ICMP_NEQU   r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::ICMP_LT:
            std::println("{}  ICMP_LT     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::ICMP_GT:
            std::println("{}  ICMP_GT     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::ICMP_LTE:
            std::println("{}  ICMP_LTE    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::ICMP_GTE:
            std::println("{}  ICMP_GTE    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::F32CMP_EQU:
            std::println("{}  F32CMP_EQU    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F32CMP_NEQU:
            std::println("{}  F32CMP_NEQU   r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F32CMP_LT:
            std::println("{}  F32CMP_LT     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F32CMP_GT:
            std::println("{}  F32CMP_GT     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F32CMP_LTE:
            std::println("{}  F32CMP_LTE    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F32CMP_GTE:
            std::println("{}  F32CMP_GTE    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::F64CMP_EQU:
            std::println("{}  F64CMP_EQU    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F64CMP_NEQU:
            std::println("{}  F64CMP_NEQU   r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F64CMP_LT:
            std::println("{}  F64CMP_LT     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F64CMP_GT:
            std::println("{}  F64CMP_GT     r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F64CMP_LTE:
            std::println("{}  F64CMP_LTE    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;
        case IRInstructionType::F64CMP_GTE:
            std::println("{}  F64CMP_GTE    r{}  <- r{}, r{}", prefix, ir_instruction.dst, ir_instruction.src1, ir_instruction.src2);
            break;

        case IRInstructionType::CONVERT_F32_TO_F64:
            std::println("{}  F32_TO_F64 r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
            break;
        case IRInstructionType::CONVERT_F64_TO_F32:
            std::println("{}  F64_TO_F32 r{}  <- r{}", prefix, ir_instruction.dst, ir_instruction.src1);
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


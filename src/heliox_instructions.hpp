#pragma once
#include <print>
#include <format>
#include <vector>

namespace hx
{


enum class Register: int
{
    A=0, B, C, D, SP, BP, SI, DI, R8, R9, R10, R11, R12, R13, R14, R15
};
enum class Bit64Register: int
{
    RAX=0, RBX, RCX, RDX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
};
enum class Bit32Register: int
{
    EAX=0, EBX, ECX, EDX, ESP, EBP, ESI, EDI, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D
};
enum class Bit16Register: int
{
    AX=0, BX, CX, DX, SP, BP, SI, DI, R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W
};
enum class Bit8Register: int
{
    AL=0, BL, CL, DL, SPL, BPL, SIL, DIL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B

};

inline Register param_registers[6] = 
{
    Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9
};

enum class Instruction 
{
    LOAD_INT,
    LOAD_VAR,
    LOAD_STRING,
    
    STORE, 

    CALL,

    PUSH,
    POP,

    ADD,
    SUB,
    DIV,
    MUL,

    RETURN,

};

using virtual_register = int64_t;


enum class ItemType
{
    VIRTUAL_REGISTER,
    IMMEDIATE_VALUE,
    RELATIVE_ADDRESS,
    LOOKUPTABLE_INDEX,
    PARAMETER_INDEX
};


struct Item
{
    Item(ItemType item_type, int64_t value)
        : item_type(item_type), value(value)  {}
    
    std::string get_string() const
    {
        switch (item_type)
        {
            case ItemType::VIRTUAL_REGISTER:
                return std::format("r{}", value);
            case ItemType::IMMEDIATE_VALUE:
                return std::format("{}", value);
            case ItemType::RELATIVE_ADDRESS:
                return std::format("[rbp - {}]", value);
            case ItemType::LOOKUPTABLE_INDEX:
                return std::format("TABLE({})", value);
            case ItemType::PARAMETER_INDEX:
                return std::format("p{}", value);
            default:
                //TODO ERROR
                std::println("Error getting string from item");
                exit(-1);
           

        }
        return {};
    }
    ItemType item_type;
    int64_t value;
};

enum class RegisterSize
{
    BIT64,
    BIT32,
    BIT16,
    BIT8
};


inline RegisterSize get_register_size(uint32_t byte_size)
{
    switch (byte_size)
    {
        case 8:
            return RegisterSize::BIT64;
        case 4:
            return RegisterSize::BIT32;
        case 2:
            return RegisterSize::BIT16;
        case 1:
            return RegisterSize::BIT8;

        default:
            //TODO ERROR
            std::println("Tried to fetch virtual register from invalid size: '{}'", byte_size);
            exit(-1);

    }
    
}

struct InstructionTriplet
{
    InstructionTriplet(Instruction instruction, virtual_register dst, std::vector<Item> items,
            RegisterSize reg_size)
        : instruction(instruction), dst(dst), items(items), reg_size(reg_size) {}
    Instruction instruction;
    uint32_t instruc_count;
    virtual_register dst;
    std::vector<Item> items;
    RegisterSize reg_size; 
};

struct InstructionFunction
{
    InstructionFunction(const std::string& name)
        : name(name) {}
    std::string name;
    std::vector<InstructionTriplet> instruction_triplets;
};


struct LiveRange
{
    virtual_register reg;
    uint32_t first_use;
    uint32_t last_use;
};

struct InstructionData
{
    std::vector<InstructionFunction> instruction_functions;
    std::vector<LiveRange> live_ranges;
};

inline void print_instruction(const InstructionTriplet& triplet)
{
    std::string prefix;
    prefix += std::format("{:4}\t", triplet.instruc_count);

    switch (triplet.reg_size)
    {
    case RegisterSize::BIT64:
        prefix += std::format("{:4}", "(64)");
        break;
    case RegisterSize::BIT32:
        prefix += std::format("{:4}", "(32)");

        break;
    case RegisterSize::BIT16:
        prefix += std::format("{:4}", "(16)");
        break;
    case RegisterSize::BIT8:
        prefix += std::format("{:4}", "(8)");
        break;
    default:
        // TODO ERROR
        std::println("Unknown register size in print_instruction");
        exit(-1);

    }
    switch (triplet.instruction)
    {
        case Instruction::LOAD_INT:
            std::println("{} LOADI r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;
        case Instruction::LOAD_STRING:
            std::println("{} LOADS r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;
        case Instruction::LOAD_VAR:
            std::println("{} LOADV r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;
        case Instruction::CALL:
            std::print("{} CALL  r{} ", prefix, triplet.dst);
            for (const auto& i : triplet.items)
            {
                std::print("{} ", i.get_string());
            }
            std::println("");
            break;
        case Instruction::RETURN:
            std::println("{} RET   {}", prefix, triplet.dst);
            break;
        case Instruction::ADD:
            std::println("{} ADD   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case Instruction::SUB:
            std::println("{} SUB   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case Instruction::DIV:
            std::println("{} DIV   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case Instruction::MUL:
            std::println("{} MUL   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        
        case Instruction::STORE:
            std::println("{} STORE {} {}", prefix,
                    triplet.items[0].get_string(), triplet.items[1].get_string());  
            break;
      default:
        std::println("Instruction not implemented");
    }
}



}

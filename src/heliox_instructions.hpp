#pragma once
#include <print>
#include <format>
#include <vector>

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

inline registers param_registers[6] = 
{
    registers::DI, registers::SI, registers::D, registers::C, registers::R8, registers::R9
};

enum class instruction 
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


enum class item_type
{
    VIRTUAL_REGISTER,
    IMMEDIATE_VALUE,
    RELATIVE_ADDRESS,
    LOOKUPTABLE_INDEX,
    PARAMETER_INDEX
};


struct item
{
    item(item_type it, int64_t value)
        : it(it), value(value)  {}
    
    std::string get_string() const
    {
        switch (it)
        {
            case item_type::VIRTUAL_REGISTER:
                return std::format("r{}", value);
            case item_type::IMMEDIATE_VALUE:
                return std::format("{}", value);
            case item_type::RELATIVE_ADDRESS:
                return std::format("[rbp - {}]", value);
            case item_type::LOOKUPTABLE_INDEX:
                return std::format("TABLE({})", value);
            case item_type::PARAMETER_INDEX:
                return std::format("p{}", value);
            default:
                //TODO ERROR
                std::println("Error getting string from item");
                exit(-1);
           

        }
        return {};
    }
    item_type it;
    int64_t value;
};

enum class register_size
{
    BIT64,
    BIT32,
    BIT16,
    BIT8
};


inline register_size get_register_size(uint32_t byte_size)
{
    switch (byte_size)
    {
        case 8:
            return register_size::BIT64;
        case 4:
            return register_size::BIT32;
        case 2:
            return register_size::BIT16;
        case 1:
            return register_size::BIT8;

        default:
            //TODO ERROR
            std::println("Tried to fetch virtual register from invalid size: '{}'", byte_size);
            exit(-1);

    }
    
}

struct instruction_triplet
{
    instruction_triplet(instruction instruc, virtual_register dst, std::vector<item> items,
            register_size reg_size)
        : instruc(instruc), dst(dst), items(items), reg_size(reg_size) {}
    instruction instruc;
    uint32_t instruc_count;
    virtual_register dst;
    std::vector<item> items;
    register_size reg_size; 
};

struct instruction_function
{
    instruction_function(const std::string& name)
        : name(name) {}
    std::string name;
    std::vector<instruction_triplet> instruction_triplets;
};


struct live_range
{
    uint32_t first_use;
    uint32_t last_use;
};

struct instruction_data
{
    std::vector<instruction_function> instruction_functions;
    std::vector<live_range> live_ranges;
};

inline void print_instruction(const instruction_triplet& triplet)
{
    std::string prefix;
    prefix += std::format("{:4}\t", triplet.instruc_count);

    switch (triplet.reg_size)
    {
    case register_size::BIT64:
        prefix += std::format("{:4}", "(64)");
        break;
    case register_size::BIT32:
        prefix += std::format("{:4}", "(32)");

        break;
    case register_size::BIT16:
        prefix += std::format("{:4}", "(16)");
        break;
    case register_size::BIT8:
        prefix += std::format("{:4}", "(8)");
        break;
    default:
        // TODO ERROR
        std::println("Unknown register size in print_instruction");
        exit(-1);

    }
    switch (triplet.instruc)
    {
        case instruction::LOAD_INT:
            std::println("{} LOADI r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;
        case instruction::LOAD_STRING:
            std::println("{} LOADS r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;
        case instruction::LOAD_VAR:
            std::println("{} LOADV r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;
        case instruction::CALL:
            std::print("{} CALL  r{} ", prefix, triplet.dst);
            for (const auto& i : triplet.items)
            {
                std::print("{} ", i.get_string());
            }
            std::println("");
            break;
        case instruction::RETURN:
            std::println("{} RET   {}", prefix, triplet.dst);
            break;
        case instruction::ADD:
            std::println("{} ADD   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case instruction::SUB:
            std::println("{} SUB   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case instruction::DIV:
            std::println("{} DIV   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case instruction::MUL:
            std::println("{} MUL   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        
        case instruction::STORE:
            std::println("{} STORE {} {}", prefix,
                    triplet.items[0].get_string(), triplet.items[1].get_string());  
            break;
      default:
        std::println("Instruction not implemented");
    }
}



}

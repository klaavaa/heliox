#pragma once
#include <print>
#include <format>
#include <vector>

namespace hx
{


enum class Register: int
{
    NOREG=-1, A, B, C, D, SP, BP, SI, DI, R8, R9, R10, R11, R12, R13, R14, R15
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

enum class RegisterSize
{
    BIT8,
    BIT16,
    BIT32,
    BIT64
};

inline std::string register_to_string(Register reg, RegisterSize reg_size)
{
    switch (reg_size)
    {
        case RegisterSize::BIT64:
            switch (reg)
            {
                case Register::A:   return "rax";
                case Register::B:   return "rbx";
                case Register::C:   return "rcx";
                case Register::D:   return "rdx";
                case Register::SP:  return "rsp";
                case Register::BP:  return "rbp";
                case Register::SI:  return "rsi";
                case Register::DI:  return "rdi";
                case Register::R8:  return "r8"; 
                case Register::R9:  return "r9"; 
                case Register::R10: return "r10";
                case Register::R11: return "r11";
                case Register::R12: return "r12";
                case Register::R13: return "r13";
                case Register::R14: return "r14";
                case Register::R15: return "r15";
                default: return "NOT A REGISTER";
            }
        case RegisterSize::BIT32:
            switch (reg)
            {
                case Register::A:   return "eax";
                case Register::B:   return "ebx";
                case Register::C:   return "ecx";
                case Register::D:   return "edx";
                case Register::SP:  return "esp";
                case Register::BP:  return "ebp";
                case Register::SI:  return "esi";
                case Register::DI:  return "edi";
                case Register::R8:  return "r8d"; 
                case Register::R9:  return "r9d"; 
                case Register::R10: return "r10d";
                case Register::R11: return "r11d";
                case Register::R12: return "r12d";
                case Register::R13: return "r13d";
                case Register::R14: return "r14d";
                case Register::R15: return "r15d";
                default: return "NOT A REGISTER";
            }
        case RegisterSize::BIT16:
            switch (reg)
            {
                case Register::A:   return "ax";
                case Register::B:   return "bx";
                case Register::C:   return "cx";
                case Register::D:   return "dx";
                case Register::SP:  return "sp";
                case Register::BP:  return "bp";
                case Register::SI:  return "si";
                case Register::DI:  return "di";
                case Register::R8:  return "r8w"; 
                case Register::R9:  return "r9w"; 
                case Register::R10: return "r10w";
                case Register::R11: return "r11w";
                case Register::R12: return "r12w";
                case Register::R13: return "r13w";
                case Register::R14: return "r14w";
                case Register::R15: return "r15w";
                default: return "NOT A REGISTER";
            }
        case RegisterSize::BIT8:
            switch (reg)
            {
                case Register::A:   return "al";
                case Register::B:   return "bl";
                case Register::C:   return "cl";
                case Register::D:   return "dl";
                case Register::SP:  return "spl";
                case Register::BP:  return "bpl";
                case Register::SI:  return "sil";
                case Register::DI:  return "dil";
                case Register::R8:  return "r8b"; 
                case Register::R9:  return "r9b"; 
                case Register::R10: return "r10b";
                case Register::R11: return "r11b";
                case Register::R12: return "r12b";
                case Register::R13: return "r13b";
                case Register::R14: return "r14b";
                case Register::R15: return "r15b";
                default: return "NOT A REGISTER";
            }
    }
}

inline Register param_registers[6] = 
{
    Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9
};

enum class Instruction 
{
    LOAD_INT,
    LOAD_VAR,
    LOAD_STRING,
    LOAD_PARAM,
     
    STORE, 
    ZERO_DX,
    CALL,
    
    SAVE_CALLER,
    LOAD_CALLER,

    SAVE_CALLEE,
    LOAD_CALLEE,

    ALIGN,
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
    STRINGTABLE_INDEX,
    FUNCTIONTABLE_INDEX,
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
            case ItemType::STRINGTABLE_INDEX:
                return std::format("STRING_TABLE({})", value);
            case ItemType::FUNCTIONTABLE_INDEX:
                return std::format("FUNCTION_TABLE({})", value);
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


inline uint32_t get_byte_size_from_register_size(RegisterSize reg_size)
{
    switch (reg_size)
    {
        case RegisterSize::BIT64:
            return 8; 
        case RegisterSize::BIT32:
            return 4; 
        case RegisterSize::BIT16:
            return 2; 
        case RegisterSize::BIT8:
            return 1; 
    }

}

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



struct LiveRange
{
    virtual_register reg;
    RegisterSize reg_size;
    uint32_t first_use;
    uint32_t last_use;
};

struct InstructionFunction
{
    InstructionFunction(const std::string& name, bool is_extern=false)
        : name(name), is_extern(is_extern) {}
    std::string name;
    bool is_extern;
    std::vector<InstructionTriplet> instruction_triplets;
    std::vector<LiveRange> live_ranges;
};

struct InstructionData
{
    std::vector<InstructionFunction> instruction_functions;
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
        case Instruction::LOAD_PARAM:
            std::println("{} LOADP r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
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
            std::println("{} RET   r{}", prefix, triplet.dst);
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
            std::println("{} STORE r{} {}", prefix,
                    triplet.dst, triplet.items[0].get_string());  
            break;
        case Instruction::PUSH:
            std::println("{} PUSH  r{}", prefix,
                    triplet.dst);  
            break;
        case Instruction::ALIGN:
            std::println("{} ALIGN {}", prefix,
                    triplet.items[0].get_string());  
            break;
        case Instruction::ZERO_DX:
            std::println("{} ZERO  r{}", prefix,
                    triplet.dst);  
            break;
            
        case Instruction::SAVE_CALLER:
            std::println("{} SAVE_CALLER", prefix);  
            break;

        case Instruction::LOAD_CALLER:
            std::println("{} LOAD_CALLER", prefix);  
            break;
            
        case Instruction::SAVE_CALLEE:
            std::println("{} SAVE_CALLEE", prefix);  
            break;

        case Instruction::LOAD_CALLEE:
            std::println("{} LOAD_CALLEE", prefix);  
            break;
      default:
        std::println("Instruction not implemented");
    }
}



}

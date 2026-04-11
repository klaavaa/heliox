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

enum class Instruction 
{
    LOAD_INT,
    LOAD_FLOAT,
    LOAD_VAR,
    LOAD_STRING,
    LOAD_PARAM,
     
    STORE, 
    ZERO_DX,
    CALL,
    
    ALIGN,
    PUSH,
    POP,

    ADD,
    SUB,
    DIV,
    MUL,
    MOD,
    
    NEG,
    DEREF,

    IS_EQUAL,
    NOT_EQUAL,
    GREATER_THAN,
    GREATER_OR_EQUAL_THAN,
    LESS_THAN,
    LESS_OR_EQUAL_THAN,
     
    LOGICAL_AND_TEST_LEFT,
    LOGICAL_AND_TEST_RIGHT,
    LOGICAL_OR_TEST_LEFT,
    LOGICAL_OR_TEST_RIGHT,
    LOGICAL_NOT,


    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    BITWISE_NOT,

    RETURN,
    
    IF,
    ELSE,
    ENDIF,

    WHILE,
    WHILE_JUMPEND,
    ENDWHILE,
    
};

using virtual_register = int64_t;


enum class ItemType
{
    VIRTUAL_REGISTER,
    IMMEDIATE_VALUE,
    RELATIVE_ADDRESS,
    STRINGTABLE_INDEX,
    FLOATTABLE_INDEX,
    FUNCTIONTABLE_INDEX,
    PARAMETER_INDEX
};


struct Item
{
    Item(ItemType item_type, int64_t value)
        : item_type(item_type), value(value) {}
    
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
            case ItemType::FLOATTABLE_INDEX:
                return std::format("FLOAT_TABLE({})", value);
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


struct ReservedRegister
{
    union {
    Register reg;
    int32_t stack_position;
    };
    bool on_stack = false;
    std::vector<Register> reserved_without_vr;
};
struct InstructionTriplet
{
    InstructionTriplet(Instruction instruction, virtual_register dst, std::vector<Item> items, type_data type)
        : instruction(instruction), dst(dst), items(items), type(type)
    {}
    Instruction instruction;
    uint32_t instruc_count;
    virtual_register dst;
    std::vector<Item> items;
    
    type_data type;
};

struct LiveRange
{
    uint32_t first_use;
    uint32_t last_use;
};

using ReservedRegisters = std::unordered_map<virtual_register, ReservedRegister>;
using LiveRanges = std::map<virtual_register, LiveRange>;
using InstructionTriplets = std::vector<InstructionTriplet>;
using VirtualRegisterRegSizes = std::unordered_map<virtual_register, RegisterSize>;

struct InstructionFunction
{
    InstructionFunction(const std::string& name, bool is_extern=false)
        : name(name), is_extern(is_extern) {}
    std::string name;
    bool is_extern;
    InstructionTriplets instruction_triplets;
    LiveRanges live_ranges;
    ReservedRegisters reserved_registers;
    VirtualRegisterRegSizes vr_reg_sizes;
    int32_t allocated_stack;
};

struct InstructionData
{
    std::vector<InstructionFunction> instruction_functions;
};

inline void print_instruction(const InstructionTriplet& triplet)
{
    std::string prefix;
    prefix += std::format("{:4}\t", triplet.instruc_count);

    switch (get_register_size(triplet.type))
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
    case RegisterSize::BIT0:
        prefix += std::format("{:4}", "(0)");
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
        case Instruction::LOAD_FLOAT:
            std::println("{} LOADF r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
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

        case Instruction::MOD:
            std::println("{} MOD   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case Instruction::MUL:
            std::println("{} MUL   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
       
        case Instruction::NEG:
            std::println("{} NEG   r{}", prefix, triplet.dst);
            break;
        case Instruction::DEREF:
            std::println("{} DEREF r{} {}", prefix, triplet.dst, triplet.items[0].get_string());
            break;

        case Instruction::IS_EQUAL:
            std::println("{} CEQU  r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case Instruction::NOT_EQUAL:
            std::println("{} CNEQU r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case Instruction::GREATER_THAN:
            std::println("{} CGT   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case Instruction::GREATER_OR_EQUAL_THAN:
            std::println("{} CGTE  r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case Instruction::LESS_THAN:
            std::println("{} CLT   r{} {}", prefix, triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case Instruction::LESS_OR_EQUAL_THAN:
            std::println("{} CLTE  r{} {}", prefix, triplet.dst, 
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
        case Instruction::IF:
            std::println("{} IF    r{} {}", prefix, triplet.items[1].value, triplet.items[0].value);  
            break;
        case Instruction::ELSE:
            std::println("{} ELSE   {}", prefix, triplet.items[0].value);  
            break;
        case Instruction::ENDIF:
            std::println("{} ENDIF  {}", prefix, triplet.items[0].value);  
            break;
        case Instruction::WHILE:
            std::println("{} WHILE  {}", prefix, triplet.items[0].value);  
            break;
        case Instruction::WHILE_JUMPEND:
            std::println("{} WHIJZ r{} {}", prefix, triplet.items[1].value, triplet.items[0].value);  
            break;
        case Instruction::ENDWHILE:
            std::println("{} ENDWH  {}", prefix, triplet.items[0].value);  
            break;
        case Instruction::BITWISE_AND:
            std::println("{} BTAND r{} {}", prefix, triplet.dst, triplet.items[0].get_string());  
            break;
        case Instruction::BITWISE_OR:
            std::println("{} BTOR  r{} {}", prefix, triplet.dst, triplet.items[0].get_string());  
            break;
        case Instruction::BITWISE_XOR:
            std::println("{} BTXOR r{} {}", prefix, triplet.dst, triplet.items[0].get_string());  
            break;
        case Instruction::LOGICAL_AND_TEST_LEFT:
            std::println("{} LEAND {}", prefix, triplet.items[0].get_string());  
            break;
        case Instruction::LOGICAL_AND_TEST_RIGHT:
            std::println("{} RIAND {}", prefix, triplet.items[0].get_string());  
            break;
        case Instruction::LOGICAL_OR_TEST_LEFT:
            std::println("{} LEOR  {}", prefix, triplet.items[0].get_string());  
            break;
        case Instruction::LOGICAL_OR_TEST_RIGHT:
            std::println("{} RIOR {}", prefix, triplet.items[0].get_string());  
            break;


      default:
        std::println("Instruction not implemented");
    }
}



}

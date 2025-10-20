#pragma once
#include <print>
#include <format>
#include <queue>

namespace hx
{
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
        : instruc(instruc), dst(dst), items(items), reg_size(reg_size)  {}
    instruction instruc;
    virtual_register dst;
    std::vector<item> items;
    register_size reg_size; 
};

struct instruction_function
{
    instruction_function(const std::string& name)
        : name(name) {}
    std::string name;
    std::queue<instruction_triplet> triplet_queue;
};

inline void print_instruction(const instruction_triplet& triplet)
{
    std::string prefix;
    switch (triplet.reg_size)
    {
    case register_size::BIT64:
        prefix = "(64)";
        break;
    case register_size::BIT32:
        prefix = "(32)";
        break;
    case register_size::BIT16:
        prefix = "(16)";
        break;
    case register_size::BIT8:
        prefix = " (8)";
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
            std::println("{} RET   {}", prefix, triplet.items[0].get_string());
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

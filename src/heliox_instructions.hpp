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
    
    LET,

    CALL,

    PUSH,
    POP,

    ADD,
    SUB,
    DIV,
    MUL,

    RETURN,

};




// depends on instruciton:
// LOADI takes a int literal value
// LOADS takes a relative stack position

using virtual_register = int64_t;

enum class item_type
{
    VIRTUAL_REGISTER,
    IMMEDIATE_VALUE,
    RELATIVE_ADDRESS,
    LOOKUPTABLE_INDEX
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
            
        }
        return {};
    }
    item_type it;
    int64_t value;
};

struct instruction_triplet
{
    instruction_triplet(instruction instruc, virtual_register dst, std::vector<item> items)
        : instruc(instruc), dst(dst), items(items) {}
    instruction instruc;
    virtual_register dst;
    std::vector<item> items;
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
    switch (triplet.instruc)
    {
        case instruction::LOAD_INT:
            std::println("LOADI r{} {}", triplet.dst, triplet.items[0].get_string());
            break;
        case instruction::LOAD_STRING:
            std::println("LOADS r{} {}", triplet.dst, triplet.items[0].get_string());
            break;
        case instruction::LOAD_VAR:
            std::println("LOADV r{} {}", triplet.dst, triplet.items[0].get_string());
            break;
        case instruction::CALL:
            std::print("CALL  r{} ", triplet.dst);
            for (const auto& i : triplet.items)
            {
                std::print("{} ", i.get_string());
            }
            std::println("");
            break;
        case instruction::RETURN:
            std::println("RET   {}", triplet.items[0].get_string());
            break;
        case instruction::ADD:
            std::println("ADD   r{} {}", triplet.dst, 
                    triplet.items[0].get_string());
            break;
        case instruction::SUB:
            std::println("SUB   r{} {}", triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case instruction::DIV:
            std::println("DIV   r{} {}", triplet.dst, 
                    triplet.items[0].get_string());
            break;

        case instruction::MUL:
            std::println("MUL   r{} {}", triplet.dst, 
                    triplet.items[0].get_string());
            break;
        
        case instruction::LET:
            std::println("LET   {} {}", 
                    triplet.items[0].get_string(), triplet.items[1].get_string());  
            break;
      default:
        std::println("Instruction not implemented");
    }
}



}

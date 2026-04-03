#include "heliox_codegen.hpp"
#include <format>

namespace hx
{
    
CodeGeneration::CodeGeneration(sptr<SymbolTable> global_table, sptr<FunctionDataInfoMap> function_data_info_map)
    : global_table(global_table), function_data_info_map(function_data_info_map)
{
}

std::string CodeGeneration::generate(InstructionData& instruction_data)
{
    generate_data_section();

    text_section += "section .text\n";

    for (auto& func : instruction_data.instruction_functions)
    {
        if (func.is_extern)
        {
            externs += std::format("extern {}\n", func.name);
            continue;
        }
        current_func_vr_locations = function_data_info_map->at(func.name).location_map;
        text_section += emit_instruction_function(func); 
    }

    return externs + data_section + bss_section + text_section;
}

void CodeGeneration::generate_data_section()
{
    data_section = "section .data\n";
    // strings
    for (auto [index, string] :  global_table->get_string_table())
    {
        data_section += std::format("\t@str{} db \"{}\", 0\n", index, string);
    }

}

std::string CodeGeneration::emit_instruction_function(InstructionFunction& instruc_func)
{
    std::string body = std::format("global {}\n{}:\n", instruc_func.name, instruc_func.name);
    body += "\tpush rbp\n\tmov rbp, rsp\n";

    int64_t stack_allocated_memory = -function_data_info_map->at(instruc_func.name).stack_allocated_memory;
    if (stack_allocated_memory != 0)
        body += std::format("\tsub rsp, {}\n", -function_data_info_map->at(instruc_func.name).stack_allocated_memory);

    for (auto& triplet : instruc_func.instruction_triplets)
    {
        body += emit_instruction_triplet(triplet);
    }
    return body;
}

std::string CodeGeneration::emit_instruction_triplet(InstructionTriplet& triplet)
{
    switch (triplet.instruction)
    {
    case Instruction::STORE:
        return std::format("\tmov {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::PUSH:
        return std::format("\tpush {}\n", get_location(triplet.dst));
    case Instruction::ADD: 
        return std::format("\tadd {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0])); 
    case Instruction::SUB: 
        return std::format("\tsub {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0])); 
    case Instruction::MUL:
        return std::format("\timul {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::DIV:
        return std::format("\tidiv {}\n", get_location(triplet.items[0]));
    case Instruction::LOAD_INT:
        return std::format("\tmov {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::LOAD_PARAM:
        return "";
    case Instruction::LOAD_STRING:
        return std::format("\tlea {}, [rel {}]\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::RETURN:
        return std::format("\tmov rax, {}\n\tmov rsp, rbp\n\tpop rbp\n\tret\n", get_location(triplet.dst));
    case Instruction::ALIGN:
        return std::format("\tadd rsp, {}\n", get_location(triplet.items[0]));
    case Instruction::CALL:
        return std::format("\tcall {}\n", global_table->get_function_name_from_id(triplet.items[0].value));
    case Instruction::ZERO_DX:
        return std::format("\txor {}, {}\n", get_location(triplet.dst), get_location(triplet.dst));
    default:
        return "not yet implemented\n";
        //TODO ERROR
        //std::println("Instruction not implemented");
        //exit(-1);

    }

}

std::string CodeGeneration::get_location(Item item)
{
    switch (item.item_type)
    {
        case ItemType::VIRTUAL_REGISTER: 
            return get_location(static_cast<virtual_register>(item.value));
        case ItemType::IMMEDIATE_VALUE:
            return std::format("{}", item.value);
        case ItemType::RELATIVE_ADDRESS:
            std::println("ITEMTYPE RELATIVE ADDRESS, EXITING");
            exit(-1);
        case ItemType::STRINGTABLE_INDEX:
            return std::format("@str{}", item.value);
        case ItemType::FUNCTIONTABLE_INDEX:
            std::println("ITEMTYPE FUNCTION_TABLE INDEX, EXITING");
            exit(-1);
        case ItemType::PARAMETER_INDEX:
            std::println("ITEMTYPE FUNCTION_TABLE INDEX, EXITING");
            exit(-1);
    }
}
std::string CodeGeneration::get_location(virtual_register vr)
{
    const VirtualRegisterLocation& location = current_func_vr_locations[vr];
    if (location.is_spilled)
    {
        if (location.stack_position < 0)
            return std::format("[rbp - {}]", -location.stack_position);
        else 
            return std::format("[rbp + {}]", location.stack_position);
    }

    return register_to_string(location.allocated_register);

}

}

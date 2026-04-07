#include "heliox_codegen.hpp"
#include <format>

namespace hx
{
    
CodeGeneration::CodeGeneration(sptr<SymbolTable> global_table, sptr<FunctionDataInfoMap> function_data_info_map)
    : global_table(global_table), function_data_info_map(function_data_info_map)
{
    callee_saved_registers.set(Register::B);
    callee_saved_registers.set(Register::R15);
    callee_saved_registers.set(Register::R14);
    callee_saved_registers.set(Register::R13);
    callee_saved_registers.set(Register::R12);

    caller_saved_registers.set(Register::A);
    caller_saved_registers.set(Register::C);
    caller_saved_registers.set(Register::D);
    caller_saved_registers.set(Register::SI);
    caller_saved_registers.set(Register::DI);
    caller_saved_registers.set(Register::R8);
    caller_saved_registers.set(Register::R9);
    caller_saved_registers.set(Register::R10);
    caller_saved_registers.set(Register::R11);
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
        std::string parsed_string = parse_string(string);
        data_section += std::format("\t@str{} db {}, 0\n", index, parsed_string);
    }

}

std::string CodeGeneration::parse_string(const std::string& str)
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

std::string CodeGeneration::emit_instruction_function(InstructionFunction& instruc_func)
{
    std::string body = std::format("global {}\n{}:\n", instruc_func.name, instruc_func.name);
    body += "\tpush rbp\n\tmov rbp, rsp\n";
    

    int64_t stack_allocated_memory = -function_data_info_map->at(instruc_func.name).stack_allocated_memory;
    if (stack_allocated_memory != 0)
        body += std::format("\tsub rsp, {}\n", -function_data_info_map->at(instruc_func.name).stack_allocated_memory);

    // callee saved stuff
    callee_preserved_registers.clear();
    added_padding_from_callee_save = false;
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
        return std::format("\tpush {}\n", get_location(triplet.dst, triplet.reg_size));
    case Instruction::ADD: 
        return std::format("\tadd {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0])); 
    case Instruction::SUB: 
        return std::format("\tsub {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0])); 
    case Instruction::MUL:
        return std::format("\timul {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::DIV:
        return std::format("\tidiv {}\n", get_location(triplet.items[0]));
    case Instruction::MOD:
        return std::format("\tidiv {}\n", get_location(triplet.items[0]));
    case Instruction::NEG:
        return std::format("\tneg {}\n", get_location(triplet.dst));
    case Instruction::DEREF:
        return std::format("\tmov {}, [{}]\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::BITWISE_AND:
        return std::format("\tand {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::BITWISE_OR:
        return std::format("\tor {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::BITWISE_XOR:
        return std::format("\txor {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::BITWISE_NOT:
        return std::format("\tnot {}\n", get_location(triplet.dst));
    case Instruction::LOGICAL_AND_TEST_LEFT:
        return std::format("\ttest {}, {}\n\tjz .LOGICAL_AND_FALSE{}\n", get_location(triplet.items[0]), get_location(triplet.items[0]),
                get_location(triplet.items[1]));
    case Instruction::LOGICAL_AND_TEST_RIGHT:
        return std::format("\ttest {}, {}\n\tjz .LOGICAL_AND_FALSE{}\n\tmov {}, 1\n\tjmp .LOGICAL_AND_TRUE{}\n.LOGICAL_AND_FALSE{}:\n\tmov {}, 0\n.LOGICAL_AND_TRUE{}:\n", 
                get_location(triplet.items[0]), get_location(triplet.items[0]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]));
    case Instruction::LOGICAL_OR_TEST_LEFT:
        return std::format("\ttest {}, {}\n\tjnz .LOGICAL_OR_TRUE{}\n", get_location(triplet.items[0]), get_location(triplet.items[0]),
                get_location(triplet.items[1]));
    case Instruction::LOGICAL_OR_TEST_RIGHT:
        return std::format("\ttest {}, {}\n\tjnz .LOGICAL_OR_TRUE{}\n\tmov {}, 0\n\tjmp .LOGICAL_OR_FALSE{}\n.LOGICAL_OR_TRUE{}:\n\tmov {}, 1\n.LOGICAL_OR_FALSE{}:\n", 
                get_location(triplet.items[0]), get_location(triplet.items[0]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]));
    case Instruction::LOGICAL_NOT:
        return std::format("\tcmp {}, {}\n\tsete {}\n", get_location(triplet.dst), 0, get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::LOAD_INT:
        return std::format("\tmov {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::LOAD_PARAM:
        return std::format("");
    case Instruction::LOAD_STRING:
        return std::format("\tlea {}, [rel {}]\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::RETURN:
        return std::format("\tmov rsp, rbp\n\tpop rbp\n\tret\n", get_location(triplet.dst));
    case Instruction::ALIGN:
        return std::format("\tadd rsp, {}\n", get_location(triplet.items[0]));
    case Instruction::CALL:
        return std::format("\tcall {}\n", global_table->get_function_name_from_id(triplet.items[0].value));
    case Instruction::ZERO_DX:
        return std::format("\txor {}, {}\n", get_location(triplet.dst), get_location(triplet.dst));
    case Instruction::IS_EQUAL:
        return std::format("\tcmp {}, {}\n\tsete {}\n", get_location(triplet.dst), get_location(triplet.items[0]), 
                get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::NOT_EQUAL:
        return std::format("\tcmp {}, {}\n\tsetne {}\n", get_location(triplet.dst), get_location(triplet.items[0]), 
                get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::GREATER_THAN:
        return std::format("\tcmp {}, {}\n\tsetg {}\n", get_location(triplet.dst), get_location(triplet.items[0]), 
                get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::GREATER_OR_EQUAL_THAN:
        return std::format("\tcmp {}, {}\n\tsetge {}\n", get_location(triplet.dst), get_location(triplet.items[0]), 
                get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::LESS_THAN:
        return std::format("\tcmp {}, {}\n\tsetl {}\n", get_location(triplet.dst), get_location(triplet.items[0]), 
                get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::LESS_OR_EQUAL_THAN:
        return std::format("\tcmp {}, {}\n\tsetle {}\n", get_location(triplet.dst), get_location(triplet.items[0]), 
                get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::IF:
        return std::format("\ttest {}, {}\n\tjz .ELSE{}\n", get_location(triplet.items[1]), get_location(triplet.items[1]), 
                get_location(triplet.items[0]));
    case Instruction::ELSE:
        return std::format("\tjmp .IFEND{}\n.ELSE{}:\n", get_location(triplet.items[0]), get_location(triplet.items[0]));
    case Instruction::ENDIF:
        return std::format(".IFEND{}:\n", get_location(triplet.items[0]));
    case Instruction::WHILE:
        return std::format(".WHILE{}:\n", get_location(triplet.items[0]));
    case Instruction::WHILE_JUMPEND:
        return std::format("\ttest {}, {}\n\tjz .WHILEEND{}\n", get_location(triplet.items[1]), get_location(triplet.items[1]),
                get_location(triplet.items[0]));
    case Instruction::ENDWHILE:
        return std::format("\tjmp .WHILE{}\n.WHILEEND{}:\n", get_location(triplet.items[0]),
                get_location(triplet.items[0]));

    case Instruction::SAVE_CALLER:
        {
        RegisterBitSet reserved_registers = get_reserved_registers_at(triplet.instruc_count)
                .get_bits_in_other(caller_saved_registers);
        std::string base = "";
        for (const auto reg : reserved_registers.get_available_registers())
        {
            base += std::format("\tpush {}\n", register_to_string(reg, RegisterSize::BIT64)); 
            caller_preserved_registers.push_back(reg);
        }
        if (caller_preserved_registers.size() % 2 == 1)
        {
            added_padding_from_caller_save = true;
            base += std::format("\tsub rsp, 8\n");
        }
        return base;
        }
    case Instruction::LOAD_CALLER:
        {
            std::string base = "";
            if (added_padding_from_caller_save)
            {
                base += std::format("\tadd rsp, 8\n");
                added_padding_from_caller_save = false;
            }
            for (int i = caller_preserved_registers.size()-1; i >= 0; i--)
            {
                const auto reg = caller_preserved_registers[i];
                base += std::format("\tpop {}\n", register_to_string(reg, RegisterSize::BIT64)); 
            }

            caller_preserved_registers.clear();
            return base;
        }
    case Instruction::SAVE_CALLEE:
        {
            std::string base = "";
            RegisterBitSet used_registers = get_function_used_registers();
            RegisterBitSet to_preserve = used_registers.get_bits_in_other(callee_saved_registers);
            
            for (const auto reg : to_preserve.get_available_registers())
            {
                base += std::format("\tpush {}\n", register_to_string(reg, RegisterSize::BIT64));
                callee_preserved_registers.push_back(reg);
            }
            if (callee_preserved_registers.size() % 2 == 1)
            {
                base += std::format("\tsub rsp, 8\n");
                added_padding_from_callee_save = true;
            }
            return base;
        }
    case Instruction::LOAD_CALLEE:
        {
            std::string base = "";
            if (added_padding_from_callee_save)
            {
                base += std::format("\tadd rsp, 8\n");
            }

            for (int i = callee_preserved_registers.size()-1; i >= 0; i--)
            {
                const auto reg = callee_preserved_registers[i];
                base += std::format("\tpop {}\n", register_to_string(reg, RegisterSize::BIT64)); 
            }
            return base;
        }
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
    return get_location(vr, location.live_range.reg_size);
}

std::string CodeGeneration::get_location(virtual_register vr, RegisterSize reg_size)
{
    const VirtualRegisterLocation& location = current_func_vr_locations[vr];
    if (location.is_spilled)
    {
        if (location.stack_position < 0)
            return std::format("{}[rbp - {}]", 
                    register_size_to_prefix(reg_size),
                    -location.stack_position);
        else 
            return std::format("{}[rbp + {}]",
                    register_size_to_prefix(reg_size),
                    location.stack_position);
    }
    return register_to_string(location.allocated_register, reg_size);
}

RegisterBitSet CodeGeneration::get_reserved_registers_at(int64_t position)
{
    RegisterBitSet reserved_registers;
    for (const auto [vr, loc]  : current_func_vr_locations)
    {
        if (loc.is_spilled) continue;
        if (loc.live_range.first_use < position && loc.live_range.last_use > position)
        {
            reserved_registers.set(loc.allocated_register);
        }
    }
    return reserved_registers;
}

RegisterBitSet CodeGeneration::get_function_used_registers()
{
    RegisterBitSet used_registers;
    for (const auto [vr, loc]  : current_func_vr_locations)
    {
        if (loc.is_spilled) continue;
        used_registers.set(loc.allocated_register);
    }
    return used_registers;
}

}


#include "heliox_codegen.hpp"
#include <format>

namespace hx
{
    
CodeGeneration::CodeGeneration(sptr<SymbolTable> global_table, sptr<FunctionLocationData> function_location_data)
    : global_table(global_table), function_location_data(function_location_data), instruction_type(primitive_type::VOID,0)
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
        current_func_vr_locations = function_location_data->at(func.name);
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
    
    // TODO 
    
    int32_t stack_allocated_memory = -instruc_func.allocated_stack;
    if (stack_allocated_memory != 0)
        body += std::format("\tsub rsp, {}\n", stack_allocated_memory);
    
    

    // callee saved stuff
    callee_preserved_registers.clear();
    added_padding_from_callee_save = false;
    body += save_callee();
    for (auto& triplet : instruc_func.instruction_triplets)
    {
        body += emit_instruction_triplet(triplet);
    }

    return body;
}

std::string CodeGeneration::emit_instruction_triplet(InstructionTriplet& triplet)
{
    instruction_type = triplet.type;
    switch (triplet.instruction)
    {
    case Instruction::STORE:
        return gen_instruc_safe("mov", triplet.dst, triplet.items[0]);
    case Instruction::PUSH:
        return std::format("\tpush {}\n", get_location(triplet.dst, RegisterSize::BIT64));
    case Instruction::ADD: 
        return gen_instruc_safe("add", triplet.dst, triplet.items[0]);
    case Instruction::SUB: 
        return gen_instruc_safe("sub", triplet.dst, triplet.items[0]);
    case Instruction::MUL:
        return gen_instruc_safe("imul", triplet.dst, triplet.items[0]);
    case Instruction::DIV:
        return std::format("\txor rdx, rdx\n\tidiv {}\n", get_location(triplet.items[0]));
    case Instruction::MOD:
        return std::format("\txor rdx, rdx\n\tidiv {}\n", get_location(triplet.items[1]));
    case Instruction::NEG:
        return std::format("\tneg {}\n", get_location(triplet.dst));
    case Instruction::DEREF:
        if (current_func_vr_locations.at(triplet.items[0].value).is_spilled)
            return std::format("\tmov {}, {}\n\tmov {}, [{}]\n", 
                    get_location(triplet.dst), get_location(triplet.items[0]),
                    get_location(triplet.dst), get_location(triplet.dst));
        return std::format("\tmov {}, [{}]\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::BITWISE_AND:
        return gen_instruc_safe("and", triplet.dst, triplet.items[0]);
    case Instruction::BITWISE_OR:
        return gen_instruc_safe("or", triplet.dst, triplet.items[0]);
    case Instruction::BITWISE_XOR:
        return gen_instruc_safe("xor", triplet.dst, triplet.items[0]);
    case Instruction::BITWISE_NOT:
        return std::format("\tnot {}\n", get_location(triplet.dst));
    case Instruction::LOGICAL_AND_TEST_LEFT:
        return std::format("\tcmp {}, 0\n\tjz .LOGICAL_AND_FALSE{}\n", get_location(triplet.items[0]),
                get_location(triplet.items[1]));
    case Instruction::LOGICAL_AND_TEST_RIGHT:
        return std::format("\tcmp {}, 0\n\tjz .LOGICAL_AND_FALSE{}\n\tmov {}, 1\n\tjmp .LOGICAL_AND_TRUE{}\n.LOGICAL_AND_FALSE{}:\n\tmov {}, 0\n.LOGICAL_AND_TRUE{}:\n", 
                get_location(triplet.items[0]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]));
    case Instruction::LOGICAL_OR_TEST_LEFT:
        return std::format("\tcmp {}, 0\n\tjnz .LOGICAL_OR_TRUE{}\n", get_location(triplet.items[0]),
                get_location(triplet.items[1]));
    case Instruction::LOGICAL_OR_TEST_RIGHT:
        return std::format("\tcmp {}, 0\n\tjnz .LOGICAL_OR_TRUE{}\n\tmov {}, 0\n\tjmp .LOGICAL_OR_FALSE{}\n.LOGICAL_OR_TRUE{}:\n\tmov {}, 1\n.LOGICAL_OR_FALSE{}:\n", 
                get_location(triplet.items[0]), 
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]),
                get_location(triplet.items[1]), get_location(triplet.dst), get_location(triplet.items[1]));
    case Instruction::LOGICAL_NOT:
        return std::format("\tcmp {}, 0\n\tsete {}\n", get_location(triplet.dst), get_location(triplet.dst, RegisterSize::BIT8));
    case Instruction::LOAD_INT:
        return std::format("\tmov {}, {}\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::LOAD_PARAM:
        return std::format("");
    case Instruction::LOAD_STRING:
        if (current_func_vr_locations.at(triplet.dst).is_spilled)
        {
            return std::format("\tlea {}, [rel {}]\n\tmov {}, {}\n",
                    register_to_string(scratch_register, RegisterSize::BIT64), get_location(triplet.items[0]),
                    get_location(triplet.dst), register_to_string(scratch_register, RegisterSize::BIT64));
        }
        return std::format("\tlea {}, [rel {}]\n", get_location(triplet.dst), get_location(triplet.items[0]));
    case Instruction::RETURN:
        return std::format("{}\tmov rsp, rbp\n\tpop rbp\n\tret\n", load_callee(), get_location(triplet.dst));
    case Instruction::ALIGN:
        return std::format("\tadd rsp, {}\n", get_location(triplet.items[0]));
    case Instruction::CALL:
        {
        std::string base = save_caller(triplet.instruc_count);
        base += std::format("\tcall {}\n", global_table->get_function_name_from_id((uint32_t)triplet.items[0].value));
        base += load_caller();
        return base;
        }
    case Instruction::IS_EQUAL:
        {
        std::string cmp = gen_instruc_safe("cmp", triplet.dst, triplet.items[0]);
        return std::format("{}\tsete {}\n", cmp, 
                get_location(triplet.dst, RegisterSize::BIT8));
        }
    case Instruction::NOT_EQUAL:
        {
        std::string cmp = gen_instruc_safe("cmp", triplet.dst, triplet.items[0]);
        return std::format("{}\tsetne {}\n", cmp, 
                get_location(triplet.dst, RegisterSize::BIT8));
        }
    case Instruction::GREATER_THAN:
        {
        std::string cmp = gen_instruc_safe("cmp", triplet.dst, triplet.items[0]);
        std::string inst = "setg";
        if (is_unsigned(triplet.type))
            inst = "seta";
        return std::format("{}\t{} {}\n", cmp, inst,
                get_location(triplet.dst, RegisterSize::BIT8));
        }
    case Instruction::GREATER_OR_EQUAL_THAN:
        {
        std::string cmp = gen_instruc_safe("cmp", triplet.dst, triplet.items[0]);
        std::string inst = "setge";
        if (is_unsigned(triplet.type))
            inst = "setae";
        return std::format("{}\t{} {}\n", cmp, inst,
                get_location(triplet.dst, RegisterSize::BIT8));
        }
    case Instruction::LESS_THAN:
        {
        std::string cmp = gen_instruc_safe("cmp", triplet.dst, triplet.items[0]);
        std::string inst = "setl";
        if (is_unsigned(triplet.type))
            inst = "setb";
        return std::format("{}\t{} {}\n", cmp, inst,
                get_location(triplet.dst, RegisterSize::BIT8));
        }
    case Instruction::LESS_OR_EQUAL_THAN:
        {
        std::string cmp = gen_instruc_safe("cmp", triplet.dst, triplet.items[0]);
        std::string inst = "setle";
        if (is_unsigned(triplet.type))
            inst = "setbe";
        return std::format("{}\t{} {}\n", cmp, inst,
                get_location(triplet.dst, RegisterSize::BIT8));
        }
    case Instruction::IF:
        return std::format("\tcmp {}, 0\n\tjz .ELSE{}\n", get_location(triplet.items[1]), 
                get_location(triplet.items[0]));
    case Instruction::ELSE:
        return std::format("\tjmp .IFEND{}\n.ELSE{}:\n", get_location(triplet.items[0]), get_location(triplet.items[0]));
    case Instruction::ENDIF:
        return std::format(".IFEND{}:\n", get_location(triplet.items[0]));
    case Instruction::WHILE:
        return std::format(".WHILE{}:\n", get_location(triplet.items[0]));
    case Instruction::WHILE_JUMPEND:
        return std::format("\tcmp {}, 0\n\tjz .WHILEEND{}\n", get_location(triplet.items[1]),
                get_location(triplet.items[0]));
    case Instruction::ENDWHILE:
        return std::format("\tjmp .WHILE{}\n.WHILEEND{}:\n", get_location(triplet.items[0]),
                get_location(triplet.items[0]));
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
            std::println("ITEMTYPE PARAMETER_TABLE INDEX, EXITING");
            exit(-1);
        case ItemType::FLOATTABLE_INDEX:
            std::println("ITEMTYPE FLOAT_TABLE INDEX, EXITING");
            exit(-1);
    }
}
std::string CodeGeneration::get_location(virtual_register vr)
{
    return get_location(vr, get_register_size(instruction_type));
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

RegisterBitSet CodeGeneration::get_reserved_registers_at(uint32_t position)
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

std::string CodeGeneration::save_caller(uint32_t instruc_count)
{
    std::println("INSTRUCT COUNT {}", instruc_count);
    RegisterBitSet reserved_registers = get_reserved_registers_at(instruc_count)
            .get_bits_in_other(g_register_data.caller_saved_registers);
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
std::string CodeGeneration::load_caller()
{

    std::string base = "";
    if (added_padding_from_caller_save)
    {
        base += std::format("\tadd rsp, 8\n");
        added_padding_from_caller_save = false;
    }
    for (int i = (int)caller_preserved_registers.size()-1; i >= 0; i--)
    {
        const auto reg = caller_preserved_registers[i];
        base += std::format("\tpop {}\n", register_to_string(reg, RegisterSize::BIT64)); 
    }

    caller_preserved_registers.clear();
    return base;
}
std::string CodeGeneration::save_callee()
{
    std::string base = "";
    RegisterBitSet used_registers = get_function_used_registers();
    RegisterBitSet to_preserve = used_registers.get_bits_in_other(g_register_data.callee_saved_registers);
    
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
std::string CodeGeneration::load_callee()
{
    std::string base = "";
    if (added_padding_from_callee_save)
    {
        base += std::format("\tadd rsp, 8\n");
    }

    for (int i = (int)callee_preserved_registers.size()-1; i >= 0; i--)
    {
        const auto reg = callee_preserved_registers[i];
        base += std::format("\tpop {}\n", register_to_string(reg, RegisterSize::BIT64)); 
    }
    return base;
}

std::string CodeGeneration::gen_instruc_safe(const std::string inst, virtual_register dst, Item arg)
{
    std::string base = "";
    if (get_register_size(instruction_type) == RegisterSize::BIT8)
    {
        base += std::format("\tand {}, {}\n", get_location(dst, RegisterSize::BIT64), "0xFF");
    }
    if (arg.item_type != ItemType::VIRTUAL_REGISTER)
        return base + std::format("\t{} {}, {}\n", inst, get_location(dst), get_location(arg));

    if (current_func_vr_locations.at(dst).is_spilled && current_func_vr_locations.at(arg.value).is_spilled) 
    {
        std::string scratch = register_to_string(scratch_register, get_register_size(instruction_type));
        return base + std::format("\t{} {}, {}\n\t{} {}, {}\n", inst, scratch, get_location(arg),
                inst, get_location(dst), scratch);
    }
    return base + std::format("\t{} {}, {}\n", inst, get_location(dst), get_location(arg));
}

}


#include "heliox_linearscan.hpp"

namespace hx
{

LinearScanRegisterAllocation::LinearScanRegisterAllocation(InstructionData instruction_data, sptr<SymbolTable> global_table)
    :  instruction_data(instruction_data), global_table(global_table)
{
    function_data_info_map = std::make_shared<FunctionDataInfoMap>();
    register_set.set(Register::A);
    register_set.set(Register::B);
    register_set.set(Register::C);
    register_set.set(Register::D);
    register_set.set(Register::DI);
    register_set.set(Register::SI);
    register_set.set(Register::R8);
    register_set.set(Register::R9);
    register_set.set(Register::R10);
    register_set.set(Register::R11);
    register_set.set(Register::R12);
    register_set.set(Register::R13);
    register_set.set(Register::R14);
    register_set.set(Register::R15);
}

void LinearScanRegisterAllocation::scan()
{
    for (const auto& instruction_function : instruction_data.instruction_functions)
    {
        reserved_active.clear();
        active.clear();
        const std::string& fname = instruction_function.name;
        // do a pre-scan to reserve certain registers at needed times (such as function call parameters)
        int param_offset = 16;
        for (const auto& triplet : instruction_function.instruction_triplets)
        {
            switch (triplet.instruction)
            {
                // this is the case for (unsigned) MUL 
                //case Instruction::MUL:
                case Instruction::RETURN:
                {
                    
                    VirtualRegisterLocation location; 
                    location.live_range = *std::find_if(instruction_function.live_ranges.begin(), instruction_function.live_ranges.end(), [triplet](LiveRange lr){ return lr.reg == triplet.dst; });
                    location.allocated_register = Register::A;
                    (*function_data_info_map)[fname].location_map[triplet.dst] = location;
                    reserved_active.push_back(location);
                    break;
                }
                case Instruction::CALL:
                {

                    for (size_t i = 1; i < triplet.items.size(); i++)                      
                    {
                        if ((i-1) >= integer_arguments_registers.size()) continue;
                        // RESERVE REGISTERS TO PASS FUNCTION PARAMS TO
                        const auto& item = triplet.items[i];
                        VirtualRegisterLocation location; 
                        location.live_range = *std::find_if(instruction_function.live_ranges.begin(), instruction_function.live_ranges.end(), [item](LiveRange lr){ return lr.reg == item.value; });
                        location.allocated_register = integer_arguments_registers[i-1];
                        (*function_data_info_map)[fname].location_map[item.value] = location;
                        reserved_active.push_back(location);
                    }
                    VirtualRegisterLocation location; 
                    //location.live_range.reg = triplet.dst;
                    location.live_range = *std::find_if(instruction_function.live_ranges.begin(), instruction_function.live_ranges.end(), [triplet](LiveRange lr){ return lr.reg == triplet.dst; });
                    location.allocated_register = Register::A;
                    (*function_data_info_map)[fname].location_map[triplet.dst] = location;
                    reserved_active.push_back(location);
                    break;
                }
                case Instruction::LOAD_PARAM:
                    {
                    VirtualRegisterLocation location; 
                    location.live_range = *std::find_if(instruction_function.live_ranges.begin(), instruction_function.live_ranges.end(), [triplet](LiveRange lr){ return lr.reg == triplet.dst; });
                    //location.live_range.reg = triplet.dst;
                    if (triplet.items[0].value < integer_arguments_registers.size()) 
                    {
                        location.allocated_register = integer_arguments_registers[triplet.items[0].value];
                    }
                    else
                    {
                        location.is_spilled = true;
                        location.stack_position = param_offset;
                        param_offset += 8;
                    }
                    (*function_data_info_map)[fname].location_map[triplet.dst] = location;
                    reserved_active.push_back(location);
                    break;
                    }
                case Instruction::ZERO_DX:
                    {
                    VirtualRegisterLocation location; 
                    location.live_range = *std::find_if(instruction_function.live_ranges.begin(), instruction_function.live_ranges.end(), [triplet](LiveRange lr){ return lr.reg == triplet.dst; });
                    location.allocated_register = Register::D;
                    (*function_data_info_map)[fname].location_map[triplet.dst] = location;
                    reserved_active.push_back(location);
                    break;
                    }
                case Instruction::DIV:
                    {
                    VirtualRegisterLocation location; 
                    location.live_range = *std::find_if(instruction_function.live_ranges.begin(), instruction_function.live_ranges.end(), [triplet](LiveRange lr){ return lr.reg == triplet.dst; });
                    location.allocated_register = Register::A;
                    (*function_data_info_map)[fname].location_map[triplet.dst] = location;
                    reserved_active.push_back(location);
                    break;
                    }
                case Instruction::LOAD_STRING:
                case Instruction::ADD:
                case Instruction::SUB:
                case Instruction::MUL:
                case Instruction::PUSH:
                {
                    vr_must_be_a_register.push_back(triplet.dst);
                    break;
                }
                case Instruction::STORE:
                {
                    auto p = std::pair<virtual_register, virtual_register>(triplet.dst, triplet.items[0].value);
                    one_vr_must_be_a_register.push_back(p);
                    break;
                }

                default:
                    break;
            }
        }
        
        // do the actual scan 
        local_stack_offset = 0;
        for (const auto& live_range : instruction_function.live_ranges)
        {
            std::sort(reserved_active.begin(), reserved_active.end(),
                    [](VirtualRegisterLocation a, VirtualRegisterLocation b) { return a.live_range.last_use < b.live_range.last_use; });
            std::sort(active.begin(), active.end(), [](VirtualRegisterLocation a, VirtualRegisterLocation b) { return a.live_range.last_use < b.live_range.last_use; });
            if (live_range.reg == -1) continue;
            if ((*function_data_info_map)[fname].location_map.contains(live_range.reg)) continue;

            expire_old_intervals(live_range);
            RegisterBitSet free_registers = register_set;
            for (auto& loc : active)
            {
                if (loc.is_spilled) continue;
                free_registers.reset(loc.allocated_register);
            }
            for (auto& loc : reserved_active)
            {
                if (loc.is_spilled) continue;
                free_registers.reset(loc.allocated_register);
            }

            if (free_registers.count() == 0)
            {
                spill_at_interval(live_range, fname);
            }
            else
            {
                Register allocated_register = free_registers.get_first_available();
                VirtualRegisterLocation location; 
                location.live_range = live_range;
                location.allocated_register = allocated_register;
                (*function_data_info_map)[fname].location_map[live_range.reg] = location;
                active.push_back(location);
            }
        }
        
        // align the local stack offset by 16
        local_stack_offset = -((abs(local_stack_offset) + 15) & ~15);
        (*function_data_info_map)[fname].stack_allocated_memory = local_stack_offset;
    }
}

void LinearScanRegisterAllocation::expire_old_intervals(LiveRange i)
{
    for (auto it = active.begin(); it != active.end(); )
    {
        VirtualRegisterLocation& location = *it;
        if (location.live_range.last_use > i.first_use)
        {
            return;
        }
        it = active.erase(it);
    }
    for (auto it = reserved_active.begin(); it != reserved_active.end();)
    {
        VirtualRegisterLocation& location = *it;
        if (location.live_range.last_use > i.first_use)
        {
            return;
        }
        it = reserved_active.erase(it);
    }

}

void LinearScanRegisterAllocation::spill_at_interval(LiveRange i, const std::string& fname)
{
    // this segment is the most disguisting code i have ever written
    // ----------------------------------------------------------------- //
    bool must_be_register = std::find_if(vr_must_be_a_register.begin(),
            vr_must_be_a_register.end(), 
            [i](virtual_register vr)
            { return i.reg == vr; }) != vr_must_be_a_register.end() ? true : false;
    
    must_be_register = must_be_register || (std::find_if(one_vr_must_be_a_register.begin(), one_vr_must_be_a_register.end(),
            [this, fname, i](std::pair<virtual_register, virtual_register> p)
            { 
                if (i.reg != p.first && i.reg != p.second) return false;
                std::println("{} -> {} {}", i.reg, p.first, p.second);
                if (i.reg == p.second && function_data_info_map->at(fname).location_map.contains(p.first))
                {
                    const VirtualRegisterLocation& loc = function_data_info_map->at(fname).location_map.at(p.first);
                    if (loc.is_spilled)
                    {
                        return true; 
                    }
                }
                if (function_data_info_map->at(fname).location_map.contains(p.second))
                {
                    const VirtualRegisterLocation& loc = function_data_info_map->at(fname).location_map.at(p.second);
                    if (loc.is_spilled)
                    {
                        return true; 
                    }
                }
                return false;
            }
            ) != one_vr_must_be_a_register.end() ? true : false);
    // ----------------------------------------------------------------- //

    VirtualRegisterLocation& spill = active.back();
    if (must_be_register || spill.live_range.last_use > i.last_use)
    {
        VirtualRegisterLocation location;
        location.live_range = i;
        location.allocated_register = spill.allocated_register;
        (*function_data_info_map)[fname].location_map[i.reg] = location;
            
        (*function_data_info_map)[fname].location_map[spill.live_range.reg].is_spilled = true;
        uint32_t byte_size = get_byte_size_from_register_size(spill.live_range.reg_size);
        local_stack_offset -= byte_size;
        int not_aligned = abs(local_stack_offset) % byte_size;
        if (not_aligned != 0)
        {
            local_stack_offset -= not_aligned;
        }
        (*function_data_info_map)[fname].location_map[spill.live_range.reg].stack_position = local_stack_offset;
        active.pop_back();
        if (must_be_register)
        { 
            reserved_active.push_back((*function_data_info_map)[fname].location_map[i.reg]);
            return;
        }
        active.push_back((*function_data_info_map)[fname].location_map[i.reg]);
    }
    else
    {
        VirtualRegisterLocation location;
        location.live_range = i;
        location.is_spilled = true;
        local_stack_offset -= get_byte_size_from_register_size(location.live_range.reg_size);
        location.stack_position = local_stack_offset;
        (*function_data_info_map)[fname].location_map[i.reg] = location;

    }
}


}

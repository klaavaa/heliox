#include "heliox_linearscan.hpp"

namespace hx
{

LinearScanRegisterAllocation::LinearScanRegisterAllocation(InstructionData& instruction_data, sptr<SymbolTable> global_table)
    :  instruction_data(instruction_data), global_table(global_table)
{
    function_location_data = std::make_shared<FunctionLocationData>();
}

void LinearScanRegisterAllocation::scan()
{
    for (auto& instruction_function : instruction_data.instruction_functions)
    {
        reserved_active.clear();
        active.clear();
        const std::string& fname = instruction_function.name;
        function_location_data->insert({fname, {}});
        auto& current_locations = function_location_data->at(fname);
        int param_offset = 16;
        local_stack_offset = 0;
        for (const auto& [vr, reserved_reg] : instruction_function.reserved_registers)
        {
                VirtualRegisterLocation location; 
                location.live_range = instruction_function.live_ranges.at(vr);
                if (reserved_reg.on_stack)
                {
                    location.stack_position = reserved_reg.stack_position;
                    location.is_spilled = true;
                }
                else
                {
                    location.allocated_register = reserved_reg.reg;
                }
                current_locations.insert({vr, location});
                reserved_active.push_back({vr, location});
                for (auto reg : reserved_reg.reserved_without_vr)
                {
                    VirtualRegisterLocation loc;
                    loc.live_range = location.live_range;
                    loc.allocated_register = reg;
                    reserved_active.push_back({-1, loc});
                }
        }
        for (const auto& [vr, live_range] : instruction_function.live_ranges)
        {
            if (current_locations.contains(vr)) continue;

            std::sort(reserved_active.begin(), reserved_active.end(),
                    [](const VRLocationPair& a, const VRLocationPair& b) { return a.location.live_range.last_use < b.location.live_range.last_use; });

            std::sort(active.begin(), active.end(), [](const VRLocationPair& a, const VRLocationPair& b) { return a.location.live_range.last_use < b.location.live_range.last_use; });
            
            expire_old_intervals(live_range);
            RegisterBitSet free_registers = g_register_data.available_registers;
            for (auto& [vr1, loc1] : active)
            {
                if (loc1.is_spilled) continue;
                free_registers.reset(loc1.allocated_register);
            }

            for (auto& [vr2, loc2] : reserved_active)
            {
                if (loc2.is_spilled) continue;
                if (loc2.live_range.first_use >= live_range.last_use && 
                        loc2.live_range.last_use <= live_range.first_use) continue;
                free_registers.reset(loc2.allocated_register);
            }

            if (free_registers.count() == 0)
            {
                spill_at_interval(vr, live_range, fname, instruction_function.vr_reg_sizes);
            }
            else
            {
                Register allocated_register = free_registers.get_first_available();
                VirtualRegisterLocation location; 
                location.live_range = live_range;
                location.allocated_register = allocated_register;
                current_locations.insert({vr, location});
                active.push_back({vr, location});
            }
        }
        local_stack_offset = -((-local_stack_offset + 15) & ~15);
        instruction_function.allocated_stack = local_stack_offset; 
    }
}

void LinearScanRegisterAllocation::expire_old_intervals(LiveRange i)
{
    for (auto it = active.begin(); it != active.end(); )
    {
        VRLocationPair& pair = *it;
        if (pair.location.live_range.last_use > i.first_use)
        {
            break;
        }
        it = active.erase(it);
    }
    for (auto it = reserved_active.begin(); it != reserved_active.end();)
    {
        VRLocationPair& pair = *it;
        if (pair.location.live_range.last_use > i.first_use)
        {
            break;
        }
        it = reserved_active.erase(it);
    }

}

void LinearScanRegisterAllocation::spill_at_interval(virtual_register vr, LiveRange lr, const std::string& fname, const VirtualRegisterRegSizes& vr_sizes)
{
    VRLocationPair& spill = active.back();
    auto& current_locations = function_location_data->at(fname);

    if (spill.location.live_range.last_use > lr.last_use)
    {
        VirtualRegisterLocation location;
        location.live_range = lr;
        location.allocated_register = spill.location.allocated_register;
        current_locations.insert({vr, location});
            
        current_locations.at(spill.vr).is_spilled = true;
        uint32_t byte_size = get_byte_size_from_register_size(vr_sizes.at(spill.vr));
        local_stack_offset -= byte_size;
        
        int not_aligned = abs(local_stack_offset) % byte_size;
        if (not_aligned != 0)
        {
            local_stack_offset -= not_aligned;
        }

        current_locations.at(spill.vr).stack_position = local_stack_offset;

        active[active.size()-1] = {vr, location};
    }
    else
    {
        VirtualRegisterLocation location;
        location.live_range = lr;
        location.is_spilled = true;
        local_stack_offset -= get_byte_size_from_register_size(vr_sizes.at(vr));
        location.stack_position = local_stack_offset;
        current_locations.insert({vr, location});

    }
}

}

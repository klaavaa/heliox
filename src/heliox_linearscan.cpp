#include "heliox_linearscan.hpp"

namespace hx
{

LinearScanRegisterAllocation::LinearScanRegisterAllocation(InstructionData instruction_data, sptr<SymbolTable> global_table)
    :  instruction_data(instruction_data), global_table(global_table)
{
    //register_set.set(Register::A);
    //register_set.set(Register::C);
    //register_set.set(Register::D);
    //register_set.set(Register::R8);
    //register_set.set(Register::R9);
    register_set.set(Register::R10);
    register_set.set(Register::R11);
}

void LinearScanRegisterAllocation::scan()
{
    // do a pre-scan to reserve certain registers at needed times (such as function call parameters)
    for (const auto& instruction_function : instruction_data.instruction_functions)
    {
        local_stack_offset = 0;
        int param_offset = 16;
        for (const auto& triplet : instruction_function.instruction_triplets)
        {
            switch (triplet.instruction)
            {
                // this is the case for (unsigned) MUL 
                //case Instruction::MUL:
                //    reserved_registers[triplet.dst] = Register::A;
                //    break;
                case Instruction::CALL:
                {

                    for (size_t i = 1; i < triplet.items.size(); i++)                      
                    {
                        if ((i-1) >= integer_arguments_registers.size()) continue;
                        // RESERVE REGISTERS TO PASS FUNCTION PARAMS TO
                        const auto& item = triplet.items[i];
                        VirtualRegisterLocation location; 
                        location.live_range.reg = item.value;
                        location.allocated_register = integer_arguments_registers[i-1];
                        virtual_register_locations[item.value] = location;
                        reserved_active.push_back(location);
                    }
                    break;
                }
                case Instruction::LOAD_PARAM:
                    {
                    VirtualRegisterLocation location; 
                    location.live_range.reg = triplet.dst;
                    if (triplet.items[0].value < integer_arguments_registers.size()) 
                    {
                        location.allocated_register = integer_arguments_registers[triplet.items[0].value];
                    }
                    else
                    {
                        location.is_spilled = true;
                        location.stack_position = param_offset;
                        param_offset += 8;
                        virtual_register_locations[triplet.dst] = location;
                    }
                    virtual_register_locations[triplet.dst] = location;
                    reserved_active.push_back(location);
                    break;
                    }
                default:
                    break;
            }
        }
    }
    // do the actual scan
    for (const auto& live_range : instruction_data.live_ranges)
    {
        if (live_range.reg == -1) continue;
        if (virtual_register_locations.contains(live_range.reg)) continue;

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
            if (loc.live_range.first_use > live_range.last_use) continue;
            free_registers.reset(loc.allocated_register);
        }
        // TODO CHANGE THIS ABOMINATION
        // this is horrible, but good enough for now.
        /*
        for (auto const& [vr, reg] : reserved_registers)
        {
            const auto& vr_live_range = *std::find_if(instruction_data.live_ranges.begin(), instruction_data.live_ranges.end(), [vr](LiveRange a){return a.reg == vr;});
            if (!(vr_live_range.first_use >= live_range.last_use || vr_live_range.last_use <= live_range.first_use))
            {
                registers_reserved_in_this_range.set(reg);
            }
        }
        */

        if (free_registers.count() == 0)
        {
            spill_at_interval(live_range);
        }
        else
        {
            Register allocated_register = free_registers.get_first_available();
            VirtualRegisterLocation location; 
            location.live_range = live_range;
            location.allocated_register = allocated_register;
            virtual_register_locations[live_range.reg] = location;
            active.push_back(location);
            std::sort(active.begin(), active.end(), [](VirtualRegisterLocation a, VirtualRegisterLocation b) { return a.live_range.last_use < b.live_range.last_use; });
        }
    }
}

void LinearScanRegisterAllocation::expire_old_intervals(LiveRange i)
{
    for (auto it = active.begin(); it != active.end(); )
    {
        VirtualRegisterLocation& location = *it;
        if (location.live_range.last_use >= i.first_use)
        {
            return;
        }
        it = active.erase(it);
    }
    for (auto it = reserved_active.begin(); it != reserved_active.end();)
    {
        VirtualRegisterLocation& location = *it;
        if (location.live_range.last_use >= i.first_use)
        {
            return;
        }
        it = reserved_active.erase(it);
    }

}

void LinearScanRegisterAllocation::spill_at_interval(LiveRange i)
{
    VirtualRegisterLocation& spill = active.back();
    if (spill.live_range.last_use > i.last_use)
    {
        VirtualRegisterLocation location;
        location.live_range = i;
        location.allocated_register = spill.allocated_register;
        virtual_register_locations[i.reg] = location;
            
        virtual_register_locations[spill.live_range.reg].is_spilled = true;
        uint32_t byte_size = get_byte_size_from_register_size(spill.live_range.reg_size);
        local_stack_offset -= byte_size;
        int not_aligned = abs(local_stack_offset) % byte_size;
        if (not_aligned != 0)
        {
            local_stack_offset -= not_aligned;
        }
        virtual_register_locations[spill.live_range.reg].stack_position = local_stack_offset; 
        active.pop_back();
        active.push_back(virtual_register_locations[i.reg]);
    }
    else
    {
        VirtualRegisterLocation location;
        location.live_range = i;
        location.is_spilled = true;
        local_stack_offset -= get_byte_size_from_register_size(location.live_range.reg_size);
        location.stack_position = local_stack_offset;
        virtual_register_locations[i.reg] = location;
    }
}


}

#include "heliox_linearscan.hpp"

namespace hx
{

LinearScanRegisterAllocation::LinearScanRegisterAllocation(InstructionData instruction_data, sptr<SymbolTable> global_table)
    :  instruction_data(instruction_data), global_table(global_table)
{
    free_registers.set(Register::A);
    free_registers.set(Register::C);
    free_registers.set(Register::D);
    free_registers.set(Register::R8);
    free_registers.set(Register::R9);
    free_registers.set(Register::R10);
    free_registers.set(Register::R11);
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
                case Instruction::CALL:
                    for (size_t i = 1; i < triplet.items.size(); i++)                      
                    {
                        const auto& item = triplet.items[i];
                        // TODO CHECK IF INTEGER ARGUMENT IF NOT THEN STACK
                        if ((i-1) >= integer_arguments_registers.size())
                        {
                            // RESERVE STACK
                            VirtualRegisterLocation location; 
                            location.live_range.reg = item.value;
                            location.is_spilled = true;
                            FunctionSymbol fs = global_table->get_function_symbol_from_id(triplet.items[0].value);
                            local_stack_offset -= fs.parameter_types[i-1].byte_size;

                            location.stack_position = local_stack_offset;
                            virtual_register_locations[item.value] = location;
                        }
                        else
                        {
                            // RESERVE REG[i]
                            reserved_registers[item.value] = integer_arguments_registers[i - 1];
                        }
                    }
                    reserved_registers[triplet.dst] = Register::A;
                    break;
                case Instruction::LOAD_PARAM:
                    if (triplet.items[0].value < integer_arguments_registers.size()) 
                    {
                        reserved_registers[triplet.dst] = integer_arguments_registers[triplet.items[0].value];
                    }
                    else
                    {
                        VirtualRegisterLocation location; 
                        location.live_range.reg = triplet.dst;
                        location.is_spilled = true;
                        location.stack_position = param_offset;
                        param_offset += get_byte_size_from_register_size(triplet.reg_size);
                        virtual_register_locations[triplet.dst] = location;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    // do the actual scan
    for (auto& live_range : instruction_data.live_ranges)
    {
        if (virtual_register_locations.contains(live_range.reg)) continue;

        expire_old_intervals(live_range);
        RegisterBitSet registers_reserved_in_this_range;
        int reserved_registers_count = 0;
        // TODO CHANGE THIS ABOMINATION
        // this is horrible, but good enough for now.
        for (auto const& [vr, reg] : reserved_registers)
        {
            const auto& vr_live_range = *std::find_if(instruction_data.live_ranges.begin(), instruction_data.live_ranges.end(), [vr](LiveRange a){return a.reg == vr;});
            if (!(vr_live_range.first_use >= live_range.last_use || vr_live_range.last_use <= live_range.first_use))
            {
                reserved_registers_count++; 
                registers_reserved_in_this_range.set(reg);
            }
        }

        if (reserved_registers.contains(live_range.reg))
        {
            VirtualRegisterLocation location; 
            location.live_range = live_range;
            location.allocated_register = reserved_registers[live_range.reg];
            virtual_register_locations[live_range.reg] = location;
            
            //reserved_registers.erase(live_range.reg);

            std::sort(active.begin(), active.end(), [](VirtualRegisterLocation a, VirtualRegisterLocation b) { return a.live_range.last_use < b.live_range.last_use; });
        }

        else if (free_registers.count() == 0)
        {
            spill_at_interval(live_range);
        }
        else
        {
            RegisterBitSet currently_available_registers = free_registers.get_bits_not_in_other(registers_reserved_in_this_range);
            Register allocated_register = currently_available_registers.get_first_available();
            free_registers.reset(allocated_register);

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
        free_registers.set(location.allocated_register);
        it = active.erase(it);
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
        local_stack_offset -= get_byte_size_from_register_size(spill.live_range.reg_size);
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
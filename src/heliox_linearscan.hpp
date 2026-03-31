#pragma once
#include "heliox_instructions.hpp"
#include "heliox_pointer.hpp"
#include <unordered_map>
#include <array>
#include <optional>
#include <vector>
#include <algorithm>

namespace hx
{

struct VirtualRegisterLocation
{
    LiveRange live_range;
    union{
        Register allocated_register;
        uint32_t stack_position;
    };
    bool is_spilled = false;
};

class LinearScanRegisterAllocation
{
public:

    LinearScanRegisterAllocation(InstructionData instruction_data)
    :  instruction_data(instruction_data)
    {
        free_registers.insert(free_registers.end(), {Register::A, Register::C, Register::D, Register::R8, Register::R9, Register::R10, Register::R11});
    }

    void scan()
    {
        for (auto& live_range : instruction_data.live_ranges)
        {
            expire_old_intervals(live_range);
            if (active.size() == number_of_registers)
            {
                spill_at_interval(live_range);
            }
            else
            {
                Register allocated_register = free_registers.back();
                free_registers.pop_back();

                VirtualRegisterLocation location; 
                location.live_range = live_range;
                location.allocated_register = allocated_register;
                virtual_register_locations[live_range.reg] = location;
                active.push_back(location);
                std::sort(active.begin(), active.end(), [](VirtualRegisterLocation a, VirtualRegisterLocation b) { return a.live_range.last_use < b.live_range.last_use; });
            }
        }
    }
    void expire_old_intervals(LiveRange i)
    {
        for (auto it = active.begin(); it != active.end(); )
        {
            VirtualRegisterLocation& location = *it;
            if (location.live_range.last_use >= i.first_use)
            {
                return;
            }
            free_registers.push_back(location.allocated_register);
            it = active.erase(it);
        }

    }
    void spill_at_interval(LiveRange i)
    {
        VirtualRegisterLocation& spill = active.back();
        if (spill.live_range.last_use > i.last_use)
        {
            VirtualRegisterLocation location;
            location.live_range = i;
            location.allocated_register = spill.allocated_register;
            virtual_register_locations[i.reg] = location;
               
            spill.is_spilled = true; 
            spill.stack_position = get_next_stack_position();
            virtual_register_locations[spill.live_range.reg].is_spilled = true;
            virtual_register_locations[spill.live_range.reg].stack_position = get_next_stack_position();

            active.pop_back();
            active.push_back(virtual_register_locations[i.reg]);
        }
        else
        {
            VirtualRegisterLocation location;
            location.live_range = i;
            location.is_spilled = true;
            location.stack_position = get_next_stack_position();
            virtual_register_locations[i.reg] = location;
        }
    }

    int get_next_stack_position() {
        next_stack_position += 8;
        return next_stack_position;
    }

    std::unordered_map<virtual_register, VirtualRegisterLocation> virtual_register_locations;
private:
    const static size_t number_of_registers = 7;

    InstructionData instruction_data;

    std::vector<VirtualRegisterLocation> active;
    std::vector<Register> free_registers;

    int next_stack_position = 0; 
};

}
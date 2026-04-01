#pragma once
#include "heliox_instructions.hpp"
#include "heliox_pointer.hpp"
#include <unordered_map>
#include <array>
#include <optional>
#include <vector>
#include <algorithm>
#include <bitset>
#include <utility>

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
    struct RegisterBitSet {
        static const size_t register_count = 16;
        RegisterBitSet() = default; 
        RegisterBitSet(std::bitset<register_count> other_bits) 
        {
            bits = other_bits;   
        }
        std::bitset<register_count> bits;
        void set(Register b) { bits.set(std::to_underlying(b)); }
        void reset(Register b) { bits.reset(std::to_underlying(b)); }
        bool test(Register b) const { return bits.test(std::to_underlying(b)); }
        RegisterBitSet get_bits_not_in_other(const RegisterBitSet& other) { return {bits & ~other.bits};}

        size_t count() const { return bits.count(); }
        Register get_first_available() const 
        { 
            for (size_t i = 0; i < register_count; i++) 
            {
                if (bits.test(i)) return static_cast<Register>(i);
            } 
            return Register::NOREG; 
        }
    };

public:

    LinearScanRegisterAllocation(InstructionData instruction_data)
    :  instruction_data(instruction_data)
    {
        //free_registers.insert(free_registers.end(), {Register::A, Register::C, Register::D, Register::R8, Register::R9, Register::R10, Register::R11});
        free_registers.set(Register::A);
        free_registers.set(Register::C);
        free_registers.set(Register::D);
        free_registers.set(Register::R8);
        free_registers.set(Register::R9);
        free_registers.set(Register::R10);
        free_registers.set(Register::R11);
    }

    void scan()
    {
        // do a pre-scan to reserve certain registers at needed times (such as function call parameters)
        for (const auto& instruction_function : instruction_data.instruction_functions)
        {
            for (const auto& triplet : instruction_function.instruction_triplets)
            {
                switch (triplet.instruction)
                {
                    case Instruction::CALL:
                        for (size_t i = 1; i < triplet.items.size(); i++)                      
                        {
                            const auto& item = triplet.items[i];
                            // TODO CHECK IF INTEGER ARGUMENT IF NOT THEN STACK
                            if (i >= integer_arguments_registers.size())
                            {
                                // RESERVE STACK
                                
                            }
                            else
                            {
                                // RESERVE REG[i]
                                reserved_registers[item.value] = integer_arguments_registers[i - 1];
                            }
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
    void expire_old_intervals(LiveRange i)
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
    void spill_at_interval(LiveRange i)
    {
        VirtualRegisterLocation& spill = active.back();
        if (spill.live_range.last_use > i.last_use)
        {
            VirtualRegisterLocation location;
            location.live_range = i;
            location.allocated_register = spill.allocated_register;
            virtual_register_locations[i.reg] = location;
               
            virtual_register_locations[spill.live_range.reg].is_spilled = true;
            virtual_register_locations[spill.live_range.reg].stack_position = get_next_stack_position(spill.live_range.reg_size);

            active.pop_back();
            active.push_back(virtual_register_locations[i.reg]);
        }
        else
        {
            VirtualRegisterLocation location;
            location.live_range = i;
            location.is_spilled = true;
            location.stack_position = get_next_stack_position(location.live_range.reg_size);
            virtual_register_locations[i.reg] = location;
        }
    }

    int get_next_stack_position(RegisterSize reg_size) {
        int next_pos = next_stack_position;
        next_stack_position += get_byte_size_from_register_size(reg_size);
        return next_pos;
    }

    std::unordered_map<virtual_register, VirtualRegisterLocation> virtual_register_locations;
private:
    const std::array<Register, 6> integer_arguments_registers = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};

    InstructionData instruction_data;

    std::vector<VirtualRegisterLocation> active;

    std::unordered_map<virtual_register, Register> reserved_registers;

    RegisterBitSet free_registers;

    int next_stack_position = 0; 
};

}
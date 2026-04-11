#pragma once
#include "heliox_instructions.hpp"
#include "heliox_pointer.hpp"
#include "heliox_symbol_table.hpp"
#include "heliox_registerdata.hpp"
#include <unordered_map>
#include <array>
#include <vector>
#include <utility>
#include <algorithm>

namespace hx
{

struct VirtualRegisterLocation
{
    LiveRange live_range;
    union
    {
        Register allocated_register;
        int32_t stack_position;
    };
    bool is_spilled = false;
};

struct VRLocationPair
{
    virtual_register vr;
    VirtualRegisterLocation location;
};

using VirtualRegisterLocationMap = std::unordered_map<virtual_register, VirtualRegisterLocation>;
using FunctionLocationData = std::unordered_map<std::string, VirtualRegisterLocationMap>;


class LinearScanRegisterAllocation
{

public:

    LinearScanRegisterAllocation(InstructionData& instruction_data, sptr<SymbolTable> global_table);

    void scan();

    
    sptr<FunctionLocationData> function_location_data;

private:
    void expire_old_intervals(LiveRange i);
    void spill_at_interval(virtual_register vr, LiveRange lr, const std::string& fname, const VirtualRegisterRegSizes& vr_sizes);
    
    VirtualRegisterLocationMap& get_locations() const;
    

private:
    const std::array<Register, 6> integer_arguments_registers = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};

    InstructionData& instruction_data;

    std::vector<VRLocationPair> active;
    std::vector<VRLocationPair> reserved_active;

    sptr<SymbolTable> global_table;
    int32_t local_stack_offset = 0;

};

}

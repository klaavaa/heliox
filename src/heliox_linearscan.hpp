#pragma once
#include "heliox_instructions.hpp"
#include "heliox_pointer.hpp"
#include "heliox_symbol_table.hpp"
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
        int32_t stack_position;
    };
    bool is_spilled = false;
};

using VirtualRegisterLocationMap = std::unordered_map<virtual_register, VirtualRegisterLocation>;

struct FunctionDataInfo
{
    VirtualRegisterLocationMap location_map;
    int64_t stack_allocated_memory;
};

using FunctionDataInfoMap = std::unordered_map<std::string, FunctionDataInfo>;

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
    void flip() { bits.flip(); }
    RegisterBitSet get_bits_not_in_other(const RegisterBitSet& other) { return {bits & ~other.bits};}
    RegisterBitSet get_bits_in_other(const RegisterBitSet& other) { return {bits & other.bits};}
    

    size_t count() const { return bits.count(); }
    Register get_first_available() const 
    { 
        for (size_t i = 0; i < register_count; i++) 
        {
            if (bits.test(i)) return static_cast<Register>(i);
        } 
        return Register::NOREG; 
    }
    std::vector<Register> get_available_registers()
    {
        std::vector<Register> available_registers; 
        for (int i = 0; i < register_count; i++) 
        {
            if (bits.test(i)) available_registers.push_back(static_cast<Register>(i));
        } 
        return available_registers;
    }
};

class LinearScanRegisterAllocation
{

public:

    LinearScanRegisterAllocation(InstructionData instruction_data, sptr<SymbolTable> global_table);

    void scan();
    void expire_old_intervals(LiveRange i);
    void spill_at_interval(LiveRange i, const std::string& name);

    sptr<FunctionDataInfoMap> function_data_info_map;
private:
    const std::array<Register, 6> integer_arguments_registers = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};

    InstructionData instruction_data;

    std::vector<VirtualRegisterLocation> active;
    std::vector<VirtualRegisterLocation> reserved_active;

    RegisterBitSet register_set;

    int local_stack_offset = 0;
    sptr<SymbolTable> global_table;
};

}

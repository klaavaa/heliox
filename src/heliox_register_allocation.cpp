#include "heliox_register_allocation.hpp"
#include <algorithm>
namespace hx
{


inline int64_t align_up(int64_t offset, int64_t align)
{
    return (offset + align - 1) & ~(align - 1);
}

void RegisterAllocator::allocate_stack(IRFunction& ir_function, const int64_t vr)
{
    auto byte_size = (int64_t)ir_function.virtual_register_types.at(vr).byte_size;
    
    // for scope deallocation
    ir_function.total_stack_allocated += byte_size;
    ir_function.total_stack_allocated = align_up(ir_function.total_stack_allocated, byte_size);

    ir_function.virtual_register_locations.insert({vr, Location::Stack(ir_function.total_stack_allocated)});
}

void RegisterAllocator::allocate_register(IRFunction& ir_function, const int64_t vr, Register reg)
{
    ir_function.virtual_register_locations.insert({vr, Location::Reg(reg)});
    gp_free_registers.erase(reg);
    active_virtual_registers.push_back(vr);
}

void RegisterAllocator::expire_old_intervals(IRFunction& ir_function, const int64_t current_vr)
{
    // sort active in order of increasing end point
    std::sort(active_virtual_registers.begin(), active_virtual_registers.end(), 
        [ir_function](int64_t a, int64_t b)
        {
            return ir_function.live_ranges.at(a).end < ir_function.live_ranges.at(b).end;
        });

    for (size_t i = 0; i < active_virtual_registers.size(); i++)
    {
        int64_t vr = active_virtual_registers[i];
        if (ir_function.live_ranges.at(vr).end >= ir_function.live_ranges.at(current_vr).start)
        {
            active_virtual_registers = std::vector<int64_t>(active_virtual_registers.begin() + i, active_virtual_registers.end());
            return;
        }
        if (ir_function.virtual_register_locations.at(vr).kind == LocationKind::REGISTER)
        {
            gp_free_registers.insert(ir_function.virtual_register_locations.at(vr).reg);
        }
    }
}


void RegisterAllocator::spill(IRFunction& ir_function, const int64_t current_vr)
{
    int64_t vr_spill = active_virtual_registers.back();
    if (ir_function.live_ranges.at(vr_spill).end > ir_function.live_ranges.at(current_vr).end)
    {
        Location spill_location = ir_function.virtual_register_locations.at(vr_spill);
        Register reg = spill_location.reg;
        ir_function.virtual_register_locations.insert({current_vr, Location::Reg(reg)});

        active_virtual_registers.pop_back();

        ir_function.virtual_register_locations.erase(vr_spill);
        allocate_stack(ir_function, vr_spill);
        active_virtual_registers.push_back(current_vr);

        return;
    }

    allocate_stack(ir_function, current_vr);
}

void RegisterAllocator::allocate_registers(IRFunction& ir_function)
{
    std::vector<int64_t> active_virtual_registers;

    gp_free_registers = g_register_data.available_general_purpose_registers;

    for (auto [vr, live_range] : ir_function.live_ranges)
    {
        expire_old_intervals(ir_function, vr);
        if (active_virtual_registers.size() == g_register_data.available_general_purpose_registers.size())
        {
            spill(ir_function, vr);
            continue;
        }
        Register reg = *gp_free_registers.begin();
        allocate_register(ir_function, vr, reg);
    }

}

void RegisterAllocator::allocate_registers(IRUnit& ir_unit)
{
    for (auto& ir_func : ir_unit.ir_functions)
    {
        allocate_registers(ir_func);
    }
}


} // namespace hx
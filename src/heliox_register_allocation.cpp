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
        
    
    std::erase_if(active_virtual_registers, 
        [&ir_function, current_vr](int64_t vr) 
        {
        if (ir_function.live_ranges.at(vr).end < ir_function.live_ranges.at(current_vr).start)
            return true;
        return false;
        });

}


void RegisterAllocator::spill(IRFunction& ir_function, const int64_t current_vr)
{
    if (!active_virtual_registers.size())
    {
        allocate_stack(ir_function, current_vr);
        return;
    }
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

std::set<Register> RegisterAllocator::get_pre_reserved_registers(IRFunction& ir_function, int64_t current_vr)
{
    std::set<Register> pre_reserved_registers;
    const auto& current_live_range = ir_function.live_ranges.at(current_vr);

    for (const auto& [vr, val] : ir_function.register_reservations)
    {
        if (val.on_stack) continue;
        const auto& live_range = ir_function.live_ranges.at(vr);
        if (current_live_range.start > live_range.end || current_live_range.end < live_range.start)
            continue;
        
        pre_reserved_registers.insert(val.reg);
        for (auto r : val.non_vr_regs)
        {
            pre_reserved_registers.insert(r);
        }
    }
    return pre_reserved_registers;
}

void RegisterAllocator::allocate_registers(IRFunction& ir_function)
{
    active_virtual_registers.clear();

    for (auto [vr, live_range] : ir_function.live_ranges)
    {

        if (ir_function.register_reservations.contains(vr))
        {
            if (ir_function.register_reservations.at(vr).on_stack)
            {
                allocate_stack(ir_function, vr);
            }
            continue;
        }


        // erases pre-reserved registers from the available register pool
        auto gp_pre_reserved_registers = get_pre_reserved_registers(ir_function, vr);
        gp_free_registers.clear();
        std::set_difference(g_register_data.available_general_purpose_registers.begin(), g_register_data.available_general_purpose_registers.end(),
         gp_pre_reserved_registers.begin(), gp_pre_reserved_registers.end(), std::inserter(gp_free_registers, gp_free_registers.end()));


        expire_old_intervals(ir_function, vr);

        // erases currently reserved registers from the available register pool
        for (int64_t active_vr : active_virtual_registers)
        {
            gp_free_registers.erase(ir_function.virtual_register_locations.at(active_vr).reg);
        }



        

        if (gp_free_registers.size() == 0)
        {
            spill(ir_function, vr);
            continue;
        }
        Register reg = *gp_free_registers.begin();
        allocate_register(ir_function, vr, reg);
    }

}

void RegisterAllocator::allocate_registers()
{
    for (auto& ir_func : ir_unit.ir_functions)
    {
        preallocate_registers(ir_func);
        allocate_registers(ir_func);
    }
}


void RegisterAllocator::preallocate_registers(IRFunction& ir_function)
{
    for (auto& instruction : ir_function.instructions)
    {
        switch (instruction.type)
        {
        case IRInstructionType::IDIV:
            preallocate_register(ir_function, instruction.src1.value, Register::A, {Register::D});
            break;
        }


    }
}

void RegisterAllocator::preallocate_register(IRFunction& ir_function, const int64_t vr, Register reg, std::vector<Register> non_vr_regs)
{
    RegisterReservation res = RegisterReservation::Reg(reg);
    res.non_vr_regs = non_vr_regs;
    ir_function.register_reservations.insert({vr, res});
    ir_function.virtual_register_locations.insert({vr, Location::Reg(reg)});
}
void RegisterAllocator::preallocate_stack(IRFunction& ir_function, const int64_t vr)
{
    RegisterReservation res = RegisterReservation::Stack();
    ir_function.register_reservations.insert({vr, res});
}

} // namespace hx
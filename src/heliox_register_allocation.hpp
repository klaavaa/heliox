#pragma once
#include <set>

#include "heliox_instructions.hpp"
namespace hx
{

class RegisterAllocator
{
public:
    void allocate_registers(IRUnit& ir_unit);
private:

    void allocate_registers(IRFunction& ir_function);
    void preallocate_registers(IRFunction& ir_function);
    void preallocate_register(IRFunction& ir_function, const int64_t vr, Register reg, std::vector<Register> non_vr_regs={});
    void preallocate_stack(IRFunction& ir_function, const int64_t vr);

    void allocate_stack(IRFunction& ir_function, const int64_t vr);
    void spill(IRFunction& ir_function, const int64_t current_vr);
    void allocate_register(IRFunction& ir_function, const int64_t vr, Register reg);
    void expire_old_intervals(IRFunction& ir_function, const int64_t current_vr);

    std::set<Register> get_pre_reserved_registers(IRFunction& ir_function, int64_t current_vr);


    std::vector<int64_t> active_virtual_registers;
    std::set<Register> gp_free_registers;

};

} // namespace hx
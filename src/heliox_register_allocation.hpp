#pragma once
#include <set>

#include "heliox_instructions.hpp"
namespace hx
{

class RegisterAllocator
{
public:
    RegisterAllocator(IRUnit& _ir_unit) : ir_unit(_ir_unit) {}
    void allocate_registers();
private:

    void cleanup_pass(IRFunction& ir_function);

    void allocate_registers(IRFunction& ir_function);

    void preallocate_registers(IRFunction& ir_function);
    void preallocate_register(IRFunction& ir_function, const int64_t vr, Register reg, std::vector<Register> non_vr_regs={});
    void preallocate_stack(IRFunction& ir_function, const int64_t vr);

    void allocate_stack(IRFunction& ir_function, const int64_t vr);
    void push_stack(IRFunction& ir_function, const int64_t vr);
    void spill(IRFunction& ir_function, const int64_t current_vr);
    void allocate_register(IRFunction& ir_function, const int64_t vr, Register reg);
    void expire_old_intervals(IRFunction& ir_function, const int64_t current_vr);

    std::set<Register> get_pre_reserved_registers(IRFunction& ir_function, int64_t current_vr);

    bool is_spilled(IRFunction& ir_function, IROperand operand);

    IRUnit& ir_unit;
    std::vector<int64_t> active_virtual_registers;
    std::set<Register> gp_free_registers;

};

} // namespace hx
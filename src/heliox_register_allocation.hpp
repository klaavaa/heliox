#pragma once
#include <unordered_set>

#include "heliox_instructions.hpp"
namespace hx
{

class RegisterAllocator
{
public:
    void allocate_registers(IRUnit& ir_unit);
private:

    void allocate_stack(IRFunction& ir_function, const int64_t vr);
    void spill(IRFunction& ir_function, const int64_t current_vr);
    void allocate_register(IRFunction& ir_function, const int64_t vr, Register reg);
    void expire_old_intervals(IRFunction& ir_function, const int64_t current_vr);
    void allocate_registers(IRFunction& ir_function);

    std::vector<int64_t> active_virtual_registers;
    std::unordered_set<Register> gp_free_registers;

};

} // namespace hx
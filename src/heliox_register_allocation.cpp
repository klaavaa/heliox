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
        if (ir_function.virtual_register_locations.contains(vr)) continue;

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


    // align stack to 16 byte alignment
    ir_function.total_stack_allocated = align_up(ir_function.total_stack_allocated, 16);
}

void RegisterAllocator::cleanup_pass(IRFunction& ir_func)
{
    // pass to clean up mem, mem instructions and push non 8 bytes
    if (ir_func.instructions.empty()) return;
    std::vector<IRInstruction> fixed_instructions;
    int64_t next_vr = ir_func.virtual_register_locations.rbegin()->first + 1;
    for (auto& instruction : ir_func.instructions)
    {

        if (is_spilled(ir_func, instruction.dst) && is_spilled(ir_func, instruction.src1))
        {
            // mov instruction to temp reg
            IRInstruction mov(IRInstructionType::MOV, IROperand::Vr(next_vr), instruction.dst, IROperand::None());
            fixed_instructions.push_back(mov); 
            // set type for new vr
            ir_func.virtual_register_types.insert({next_vr, ir_func.virtual_register_types.at(instruction.src1.value)});

            // the instruction using scratch register
            fixed_instructions.push_back(IRInstruction{instruction.type, IROperand::Vr(next_vr), instruction.src1, IROperand::None()});

            // set new vr location as the scratch register
            ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});

            IRInstruction mov_back(IRInstructionType::MOV, instruction.dst, IROperand::Vr(next_vr), IROperand::None());
            fixed_instructions.push_back(mov_back); 
            next_vr++;
            continue;
        }

        if (instruction.type == IRInstructionType::ARG_PUSH)
        {
            if (ir_func.virtual_register_types.at(instruction.src1.value).byte_size != 8)
            {
                IRInstruction mov(IRInstructionType::MOV, IROperand::Vr(next_vr), instruction.src1, IROperand::None());
                ir_func.virtual_register_types.insert({next_vr, ir_func.virtual_register_types.at(instruction.src1.value)});
                fixed_instructions.push_back(mov); 

                IRInstruction arg_push(IRInstructionType::ARG_PUSH, instruction.dst, IROperand::Vr(next_vr), instruction.src2);
                fixed_instructions.push_back(arg_push); 

                ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});

                next_vr++;
                continue;
            }
        }

        fixed_instructions.push_back(instruction);

    }

    ir_func.instructions = std::move(fixed_instructions);
}

void RegisterAllocator::allocate_registers()
{
    for (auto& ir_func : ir_unit.ir_functions)
    {
        preallocate_registers(ir_func);
        allocate_registers(ir_func);
        cleanup_pass(ir_func);
    }

}

bool RegisterAllocator::is_spilled(IRFunction& ir_function, IROperand operand)
{
    switch (operand.kind)
    {
    case IROperandKind::VIRTUAL_REGISTER:
        return ir_function.virtual_register_locations.at(operand.value).kind == LocationKind::STACK;
    case IROperandKind::LITERAL_LOCATION:
        if (ir_unit.allocated_literals.at(operand.value).type == LiteralType::FUNCTION_NAME)
            return false;
        return true;
    default:
        return false;
    }
}

void RegisterAllocator::preallocate_registers(IRFunction& ir_function)
{
    // used for stack aligment stuff
    bool first_func_push_arg = true;

    for (auto& instruction : ir_function.instructions)
    {
        switch (instruction.type)
        {
        case IRInstructionType::IDIV:
            preallocate_register(ir_function, instruction.src1.value, Register::A, {Register::D});
            break;
        case IRInstructionType::RETURN:
            preallocate_register(ir_function, instruction.dst.value, Register::A);
            break;
        case IRInstructionType::MOV_ARG:
            if (!is_integer_type(ir_function.virtual_register_types.at(instruction.src1.value)))
            {
                Logger::not_implemented();
            }
            if (instruction.src2.value < (int64_t)g_register_data.register_passed_int_args.size())
            {
                preallocate_register(ir_function, instruction.dst.value, g_register_data.register_passed_int_args.at(instruction.src2.value));
            }
            else // push the arguments on the stack (already in reverse order from IR)
            {
                // todo: if reg not 8 byte then fix
                instruction.type = IRInstructionType::ARG_PUSH;
                instruction.dst = IROperand::None();
                if (first_func_push_arg)
                {
                    first_func_push_arg = false;
                    instruction.src2.value -= g_register_data.register_passed_int_args.size() - 1;
                    instruction.src2.value %= 2;
                    if (instruction.src2.value == 0)
                    {
                        instruction.src2 = IROperand::None();
                    }
                }
                else
                {
                    instruction.src2 = IROperand::None();
                }

                ir_function.live_ranges.erase(instruction.dst.value);

            }
            break;
        case IRInstructionType::FUNCTION_CALL:
            first_func_push_arg = true;
            // reserve A for return value and globber caller saved registers
            std::vector non_vr_regs(g_register_data.caller_saved_registers.begin(), g_register_data.caller_saved_registers.end());
            preallocate_register(ir_function, instruction.dst.value, Register::A, non_vr_regs);
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
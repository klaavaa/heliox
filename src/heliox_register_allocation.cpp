#include "heliox_register_allocation.hpp"
#include <algorithm>
namespace hx
{



void RegisterAllocator::allocate_stack(IRFunction& ir_function, const int64_t vr)
{
    auto byte_size = (int64_t)ir_function.virtual_register_types.at(vr).byte_size;
    
    ir_function.total_stack_allocated += byte_size;
    ir_function.total_stack_allocated = align_up(ir_function.total_stack_allocated, byte_size);

    ir_function.virtual_register_locations.insert({vr, Location::Stack(ir_function.total_stack_allocated)});
}

void RegisterAllocator::allocate_register(IRFunction& ir_function, const int64_t vr, Register reg, std::vector<int64_t>& active_virtual_registers, std::vector<int64_t>& active_unspillable_virtual_registers)
{
    ir_function.virtual_register_locations.insert({vr, Location::Reg(reg)});
    if (ir_function.register_reservations.contains(vr) && ir_function.register_reservations.at(vr).must_be_register)
        active_unspillable_virtual_registers.push_back(vr);
    else
        active_virtual_registers.push_back(vr);
}

void RegisterAllocator::expire_old_intervals(IRFunction& ir_function, const int64_t current_vr)
{
    // sort active in order of increasing end point
    std::sort(gp_active_virtual_registers.begin(), gp_active_virtual_registers.end(), 
        [ir_function](int64_t a, int64_t b)
        {
            return ir_function.live_ranges.at(a).end < ir_function.live_ranges.at(b).end;
        });
    
    std::sort(xmm_active_virtual_registers.begin(), xmm_active_virtual_registers.end(), 
        [ir_function](int64_t a, int64_t b)
        {
            return ir_function.live_ranges.at(a).end < ir_function.live_ranges.at(b).end;
        });
        
    auto erase_lambda = [&ir_function, current_vr](int64_t vr) {
        if (ir_function.live_ranges.at(vr).end < ir_function.live_ranges.at(current_vr).start)
            return true;
        return false;
    };

    std::erase_if(gp_active_virtual_registers, erase_lambda);
    std::erase_if(xmm_active_virtual_registers, erase_lambda);

    std::erase_if(gp_active_unspillable_virtual_registers, erase_lambda);

}


void RegisterAllocator::spill(IRFunction& ir_function, const int64_t current_vr, std::vector<int64_t>& active_virtual_registers, std::vector<int64_t>& active_unspillable_virtual_registers)
{
    bool must_be_register = ir_function.register_reservations.contains(current_vr) && ir_function.register_reservations.at(current_vr).must_be_register;

    if (!active_virtual_registers.size())
    {
        if (must_be_register)
        {
            Logger::not_implemented();
        }
        allocate_stack(ir_function, current_vr);
        return;
    }

    int64_t vr_spill = active_virtual_registers.back();
    if ((ir_function.live_ranges.at(vr_spill).end > ir_function.live_ranges.at(current_vr).end) || must_be_register)
    {
        Location spill_location = ir_function.virtual_register_locations.at(vr_spill);
        Register reg = spill_location.reg;

        ir_function.virtual_register_locations.insert({current_vr, Location::Reg(reg)});

        active_virtual_registers.pop_back();

        ir_function.virtual_register_locations.erase(vr_spill);
        allocate_stack(ir_function, vr_spill);

        if (!must_be_register)
            active_virtual_registers.push_back(current_vr);
        else
            active_unspillable_virtual_registers.push_back(current_vr);

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
        if (!val.reg.has_value()) continue;

        const auto& live_range = ir_function.live_ranges.at(vr);
        if (current_live_range.start > live_range.end || current_live_range.end < live_range.start)
            continue;
        
        pre_reserved_registers.insert(val.reg.value());
        for (auto r : val.non_vr_regs)
        {
            pre_reserved_registers.insert(r);
        }
    }
    return pre_reserved_registers;
}

void RegisterAllocator::allocate_registers(IRFunction& ir_function)
{
    gp_active_virtual_registers.clear();
    gp_active_unspillable_virtual_registers.clear();

    xmm_active_virtual_registers.clear();

    for (auto [vr, live_range] : ir_function.live_ranges)
    {
        if (ir_function.virtual_register_locations.contains(vr)) continue;

        if (ir_function.register_reservations.contains(vr))
        {
            if (ir_function.register_reservations.at(vr).on_stack)
            {
                allocate_stack(ir_function, vr);
            }
            if (!ir_function.register_reservations.at(vr).must_be_register)
                continue;
        }


        // erases pre-reserved registers from the available register pool
        auto pre_reserved_registers = get_pre_reserved_registers(ir_function, vr);
        std::set<Register> free_registers;
        std::set_difference(g_register_data.available_registers.begin(), g_register_data.available_registers.end(),
         pre_reserved_registers.begin(), pre_reserved_registers.end(), std::inserter(free_registers, free_registers.end()));

        xmm_free_registers.clear(); 
        gp_free_registers.clear();

        auto it = free_registers.lower_bound(Register::XMM0);
        gp_free_registers.insert(free_registers.begin(), it);
        xmm_free_registers.insert(it, free_registers.end());


        expire_old_intervals(ir_function, vr);

        // erases currently reserved registers from the available register pool
        for (int64_t gp_active_vr : gp_active_virtual_registers)
        {
            gp_free_registers.erase(ir_function.virtual_register_locations.at(gp_active_vr).reg);
        }
        for (int64_t gp_active_unspillable_vr : gp_active_unspillable_virtual_registers)
        {
            gp_free_registers.erase(ir_function.virtual_register_locations.at(gp_active_unspillable_vr).reg);
        }

        for (int64_t xmm_active_vr : xmm_active_virtual_registers)
        {
            xmm_free_registers.erase(ir_function.virtual_register_locations.at(xmm_active_vr).reg);
        }
        for (int64_t xmm_active_unspillable_vr : xmm_active_unspillable_virtual_registers)
        {
            xmm_free_registers.erase(ir_function.virtual_register_locations.at(xmm_active_unspillable_vr).reg);
        }

        type_data vr_type = get_operand_type(ir_function, IROperand::Vr(vr));
        if (is_integer_type(vr_type))
        { 
        if (gp_free_registers.size() == 0)
        {
            spill(ir_function, vr, gp_active_virtual_registers, gp_active_unspillable_virtual_registers);
            continue;
        }
        Register reg = *gp_free_registers.begin();
        allocate_register(ir_function, vr, reg, gp_active_virtual_registers, gp_active_unspillable_virtual_registers);
        }
        else if (is_float_type(vr_type))
        {
        if (xmm_free_registers.size() == 0)
        {
            spill(ir_function, vr, xmm_active_virtual_registers, xmm_active_unspillable_virtual_registers);
            continue;
        }
        Register reg = *xmm_free_registers.begin();
        allocate_register(ir_function, vr, reg, xmm_active_virtual_registers, xmm_active_unspillable_virtual_registers);
        }
        else
        {
            Logger::not_implemented();
        }
    }


    // align stack x % 16 = 8
    ir_function.total_stack_allocated = align_up(ir_function.total_stack_allocated, 8);
    if (ir_function.total_stack_allocated % 16 == 0) ir_function.total_stack_allocated += 8;
}

void RegisterAllocator::cleanup_pass(IRFunction& ir_func)
{
    // pass to clean up mem, mem instructions and push non 8 bytes
    if (ir_func.instructions.empty()) return;
    std::vector<IRInstruction> fixed_instructions;
    int64_t next_vr = ir_func.virtual_register_locations.rbegin()->first + 1;
    bool first_func_push_arg = true;
    for (size_t i = 0; i < ir_func.instructions.size(); i++)
    {
        auto& instruction = ir_func.instructions[i];
        switch (instruction.type)
        {
            case IRInstructionType::DEREF:
                if (is_spilled(ir_func, instruction.dst))
                {
                    IRInstruction deref(IRInstructionType::DEREF, IROperand::Vr(next_vr), instruction.src1, IROperand::None());
                    ir_func.virtual_register_types.insert({next_vr, ir_func.virtual_register_types.at(instruction.dst.value)});
                    ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});
                    fixed_instructions.push_back(deref);

                    IRInstruction mov(IRInstructionType::MOV, instruction.dst, IROperand::Vr(next_vr), IROperand::None());
                    fixed_instructions.push_back(mov);

                    next_vr++;
                    continue;
                }
                break;
            case IRInstructionType::STORE_MEM:
                if (is_spilled(ir_func, instruction.src1))
                {
                    IRInstruction mov(IRInstructionType::MOV, IROperand::Vr(next_vr), instruction.src1, IROperand::None());
                    fixed_instructions.push_back(mov); 

                    ir_func.virtual_register_types.insert({next_vr, ir_func.virtual_register_types.at(instruction.src1.value)});
                    ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});
                    IRInstruction store_mem(IRInstructionType::STORE_MEM, instruction.dst, IROperand::Vr(next_vr), IROperand::None());
                    fixed_instructions.push_back(store_mem); 
                    continue;
                }
                break;
            case IRInstructionType::FUNCTION_CALL:
                first_func_push_arg = true;
                break;
            case IRInstructionType::ARG_PUSH:
                do{ 
                if (!first_func_push_arg) break;
                size_t push_count = 1; 
                for (size_t j = i+1; j < ir_func.instructions.size(); j++)
                {
                    if (ir_func.instructions[j].type != IRInstructionType::ARG_PUSH) break;
                    push_count++;
                }
                if (push_count % 2)
                {
                    instruction.src2 = IROperand::Immediate(1);
                }
                first_func_push_arg = false;
                } while (false);

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
                break;
            case IRInstructionType::JMP_IF:
            case IRInstructionType::JMP_IF_NOT:
            if (is_spilled(ir_func, instruction.src1))
            {
                IRInstruction mov(IRInstructionType::MOV, IROperand::Vr(next_vr), instruction.src1, IROperand::None());
                fixed_instructions.push_back(mov);

                IRInstruction jmp_condition(instruction.type, IROperand::None(), IROperand::Vr(next_vr), instruction.src2);
                fixed_instructions.push_back(jmp_condition);

                ir_func.virtual_register_types.insert({next_vr, ir_func.virtual_register_types.at(instruction.src1.value)});
                ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});
                next_vr++;
                continue;
            }
            break;
            default:
                if (is_spilled(ir_func, instruction.src1) && is_spilled(ir_func, instruction.src2))
                {
                    IRInstruction mov(IRInstructionType::MOV, IROperand::Vr(next_vr), instruction.src1, IROperand::None());
                    fixed_instructions.push_back(mov); 

                    ir_func.virtual_register_types.insert({next_vr, ir_func.virtual_register_types.at(instruction.src1.value)});
                    ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});
                   
                    IRInstruction inst(instruction.type, instruction.dst, IROperand::Vr(next_vr), instruction.src2);
                    fixed_instructions.push_back(inst);
                    
                    next_vr++;
                    continue;
                }
                if (is_spilled(ir_func, instruction.dst) && is_spilled(ir_func, instruction.src1))
                {
                    std::println("CLEANING UP {}", (int)instruction.type);
                    // mov instruction to temp reg
                    IRInstruction mov(IRInstructionType::MOV, IROperand::Vr(next_vr), instruction.dst, IROperand::None());
                    fixed_instructions.push_back(mov); 
                    // set type for new vr
                    ir_func.virtual_register_types.insert({next_vr, get_operand_type(ir_func, instruction.src1)});

                    // the instruction using scratch register
                    fixed_instructions.push_back(IRInstruction{instruction.type, IROperand::Vr(next_vr), instruction.src1, IROperand::None()});

                    // set new vr location as the scratch register
                    ir_func.virtual_register_locations.insert({next_vr, Location::Reg(g_register_data.gp_scratch_register)});

                    IRInstruction mov_back(IRInstructionType::MOV, instruction.dst, IROperand::Vr(next_vr), IROperand::None());
                    fixed_instructions.push_back(mov_back); 
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

    int64_t register_pushed_argc = 0; 

    for (auto& instruction : ir_function.instructions)
    {
        switch (instruction.type)
        {
        case IRInstructionType::IDIV:
            preallocate_register(ir_function, instruction.src1.value, Register::A, {Register::D});
            break;
        case IRInstructionType::IMOD:
            preallocate_register(ir_function, instruction.src1.value, Register::A, {Register::D});
            break;
        case IRInstructionType::IADD:
        case IRInstructionType::ISUB:
        case IRInstructionType::IMUL:
            preallocate_some_register(ir_function, instruction.src1.value);
            break;
        case IRInstructionType::RETURN:
            if (is_float_type(get_operand_type(ir_function, instruction.src1)))
            {
                preallocate_register(ir_function, instruction.src1.value, Register::XMM0);
            }
            else
            {
                preallocate_register(ir_function, instruction.dst.value, Register::A);
            }
            break;
        case IRInstructionType::REGISTER_ARG:
            {
            if (!is_integer_type(ir_function.virtual_register_types.at(instruction.src1.value)))
            {

                if (instruction.src2.value < (int64_t)g_register_data.register_passed_float_args.size())
                {
                    preallocate_register(ir_function, instruction.src1.value, g_register_data.register_passed_float_args.at(instruction.src2.value));
                    break;
                }

                goto inst_register_arg_push; 
            }
            if (instruction.src2.value < (int64_t)g_register_data.register_passed_int_args.size())
            {
                preallocate_register(ir_function, instruction.src1.value, g_register_data.register_passed_int_args.at(instruction.src2.value));
                break;
            }

            inst_register_arg_push:
                //need to take in codegen consideration the callee saved registers
                // push rbp => +8
                // fcall => +8
                int64_t base_offset = -16;
                #ifdef _WIN32
                // shadow space on windows
                base_offset -= 32;
                #endif
                ir_function.virtual_register_locations.insert({instruction.src1.value,
                     Location::Stack(base_offset - 8 * register_pushed_argc)});     

                register_pushed_argc++;
            break;
            }
        case IRInstructionType::MOV_ARG:
            if (!is_integer_type(ir_function.virtual_register_types.at(instruction.src1.value)))
            {
                if (instruction.src2.value < (int64_t)g_register_data.register_passed_float_args.size())
                {
                    preallocate_register(ir_function, instruction.dst.value, g_register_data.register_passed_float_args.at(instruction.src2.value));
                    break;
                }
                goto inst_mov_arg_push;
            }
            if (instruction.src2.value < (int64_t)g_register_data.register_passed_int_args.size())
            {
                preallocate_register(ir_function, instruction.dst.value, g_register_data.register_passed_int_args.at(instruction.src2.value));
                break;
            }
        inst_mov_arg_push:
            ir_function.live_ranges.erase(instruction.dst.value);
            instruction.type = IRInstructionType::ARG_PUSH;
            instruction.dst = IROperand::None();
            instruction.src2 = IROperand::None();
            break;

        case IRInstructionType::FUNCTION_CALL:
        {
            // reserve A for return value and globber caller saved registers
            std::vector non_vr_regs(g_register_data.caller_saved_registers.begin(), g_register_data.caller_saved_registers.end());
            if (is_float_type(get_operand_type(ir_function, instruction.dst)))
            {
                preallocate_register(ir_function, instruction.dst.value, Register::XMM0, non_vr_regs);
            }
            else
            {
                preallocate_register(ir_function, instruction.dst.value, Register::A, non_vr_regs);
            }
            break;
        }
        case IRInstructionType::STORE_MEM:
            preallocate_some_register(ir_function, instruction.dst.value);
            break;

        case IRInstructionType::DEREF:
            preallocate_some_register(ir_function, instruction.src1.value);
            break;
        case IRInstructionType::ADDR_OF:
            preallocate_stack(ir_function, instruction.src1.value);
            break;
        }


    }
}

void RegisterAllocator::preallocate_register(IRFunction& ir_function, const int64_t vr, Register reg, std::vector<Register> non_vr_regs)
{
    if (ir_function.register_reservations.contains(vr)) ir_function.register_reservations.erase(vr);

    RegisterReservation res = RegisterReservation::Reg(reg);
    res.non_vr_regs = non_vr_regs;
    ir_function.register_reservations.insert({vr, res});
    ir_function.virtual_register_locations.insert({vr, Location::Reg(reg)});
}
void RegisterAllocator::preallocate_some_register(IRFunction& ir_function, const int64_t vr)
{
    if (ir_function.register_reservations.contains(vr)) return;
    RegisterReservation res = RegisterReservation::SomeRegister();
    ir_function.register_reservations.insert({vr, res});
}

void RegisterAllocator::preallocate_stack(IRFunction& ir_function, const int64_t vr)
{
    RegisterReservation res = RegisterReservation::Stack();
    ir_function.register_reservations.insert({vr, res});
}
type_data RegisterAllocator::get_operand_type(const IRFunction& ir_function, const IROperand operand)
{
    switch (operand.kind)
    {
    case IROperandKind::VIRTUAL_REGISTER:
        return ir_function.virtual_register_types.at(operand.value);
    case IROperandKind::LITERAL_LOCATION:
        if (ir_unit.allocated_literals.at(operand.value).type == LiteralType::STRING)
        {
            return {primitive_type::U8, 1};
        }
        else
        {
            Logger::not_implemented();
        }
    default:
        Logger::not_implemented();

    }
}

} // namespace hx
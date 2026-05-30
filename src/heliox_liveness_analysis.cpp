#include "heliox_liveness_analysis.hpp"

namespace hx
{


void perform_liveness_analysis_on_function(IRFunction& ir_function)
{
    //todo LOOP
    
    auto update_liverange = [&ir_function](IROperand op, size_t instruction_number)
    {
        auto& live_ranges = ir_function.live_ranges;
        if (!live_ranges.contains(op.value))
        {
            live_ranges.insert({op.value, LiveRange{instruction_number, instruction_number}});
        }
        else
        {
            live_ranges.at(op.value).end = instruction_number;
        }
    };

    size_t instruction_number = 0;
    for (const auto& instruction : ir_function.instructions)
    {
        auto& live_ranges = ir_function.live_ranges;
        if (instruction.dst.kind == IROperandKind::VIRTUAL_REGISTER)
        {
            update_liverange(instruction.dst, instruction_number);
        }
        if (instruction.src1.kind == IROperandKind::VIRTUAL_REGISTER)
        {
            update_liverange(instruction.src1, instruction_number);
        }
        if (instruction.src2.kind == IROperandKind::VIRTUAL_REGISTER)
        {
            update_liverange(instruction.src2, instruction_number);
        }

        instruction_number++;
    }
}


void perform_liveness_analysis_on_unit(IRUnit& ir_unit)
{
    for (auto& ir_function : ir_unit.ir_functions)
    {
        perform_liveness_analysis_on_function(ir_function);
    }
}


}
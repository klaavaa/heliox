#include "heliox_liveness_analysis.hpp"

namespace hx
{


void perform_liveness_analysis_on_function(IRFunction& ir_function)
{
    size_t instruction_number = 0;
    for (const auto& instruction : ir_function.instructions)
    {
        auto& live_ranges = ir_function.live_ranges;
        if (instruction.dst.kind == IROperandKind::VIRTUAL_REGISTER)
        {
            if (!live_ranges.contains(instruction.dst.value))
            {
                live_ranges.insert({instruction.dst.value, LiveRange{instruction_number, instruction_number}});
            }
            else
            {
                live_ranges.at(instruction.dst.value).end = instruction_number;
            }
        }
        if (instruction.src1.kind == IROperandKind::VIRTUAL_REGISTER)
        {
            live_ranges.at(instruction.src1.value).end = instruction_number;
        }
        if (instruction.src2.kind == IROperandKind::VIRTUAL_REGISTER)
        {
            live_ranges.at(instruction.src2.value).end = instruction_number;
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
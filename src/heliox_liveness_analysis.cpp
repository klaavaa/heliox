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

    std::unordered_map<int64_t, size_t> label_numbers;

    std::vector<IROperand> arg_vrs;

    for (const auto& instruction : ir_function.instructions)
    {
        auto& live_ranges = ir_function.live_ranges;

        switch (instruction.type)
        {
            case IRInstructionType::MOV_ARG:
            case IRInstructionType::MOV_VARARG:
                arg_vrs.push_back(instruction.dst);
                break;
            case IRInstructionType::FUNCTION_CALL: 
                for (auto vr : arg_vrs)
                {
                    update_liverange(vr, instruction_number);
                }
                arg_vrs.clear();
                break;

            case IRInstructionType::LABEL:
                label_numbers.insert({instruction.src2.value, instruction_number});
                break;
            case IRInstructionType::JMP:
            case IRInstructionType::JMP_IF:
            case IRInstructionType::JMP_IF_NOT:
                if (!label_numbers.contains(instruction.src2.value))
                    break;
                
                for (auto& [vr, live_range] : live_ranges)
                {
                    if (!ir_function.vrs_with_variables.contains(vr)) continue;
                    if (live_range.end > label_numbers.at(instruction.src2.value))
                    {
                        live_range.end = instruction_number;
                    }
                }
                break;

            default:
                break;
        }
        
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
        if (ir_function.instructions.empty()) continue;
        perform_liveness_analysis_on_function(ir_function);
    }
}


}

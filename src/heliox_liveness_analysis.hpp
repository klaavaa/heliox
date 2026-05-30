#pragma once

#include "heliox_program.hpp"
#include "heliox_instructions.hpp"
#include "heliox_symbol_table.hpp"

namespace hx
{


void perform_liveness_analysis_on_function(IRFunction& ir_function, sptr<SymbolTable> table);
void perform_liveness_analysis_on_unit(IRUnit& ir_unit, sptr<SymbolTable> global_table);

inline void print_live_ranges(IRUnit& ir_unit)
{
    for (auto& f: ir_unit.ir_functions)
    {
        for (auto [vr, lr] : f.live_ranges)
        {
            std::println("{}:  [{} -> {}]", vr, lr.start, lr.end);
        }
    }
}
} // namepace hx
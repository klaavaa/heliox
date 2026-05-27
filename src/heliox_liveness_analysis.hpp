#pragma once

#include "heliox_program.hpp"
#include "heliox_instructions.hpp"

namespace hx
{


void perform_liveness_analysis_on_function(IRFunction& ir_function);
void perform_liveness_analysis_on_unit(IRUnit& ir_unit);

} // namepace hx
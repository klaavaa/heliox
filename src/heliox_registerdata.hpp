#pragma once
#include <print>
#include <array> 
#include <vector>
#include <set>
#include <utility>
#include "heliox_types.hpp"

namespace hx {

enum class Register: int
{
    NOREG=-1, A, B, C, D, SP, BP, SI, DI, R8, R9, R10, R11, R12, R13, R14, R15,
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15
};

enum class RegisterSize
{
    BIT8,
    BIT16,
    BIT32,
    BIT64
};


struct RegisterData
{
    // R11 SCRATCH REGISTER 
    std::set<Register> available_general_purpose_registers = {Register::A, Register::B};
    //std::set<Register> available_general_purpose_registers = {Register::A, Register::B, Register::C,
    //        Register::D, Register::DI, Register::SI, Register::R8, Register::R9, Register::R10, Register::R11,
    //        Register::R12, Register::R13, Register::R14, Register::R15};
    
    //RegisterBitSet available_xmm_registers = RegisterBitSet(
    //        {Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3, Register::XMM4, Register::XMM5,
    //        Register::XMM6, Register::XMM7, Register::XMM8, Register::XMM9, Register::XMM10, Register::XMM11,
    //        Register::XMM12, Register::XMM13, Register::XMM14, Register::XMM15}
    //        );


    Register gp_scratch_register = Register::R11;
    Register xmm_scratch_register = Register::XMM11;
    
#ifdef _WIN32

    std::array<Register, 4> register_passed_int_args = {Register::C, Register::D, Register::R8, Register::R9};
    std::array<Register, 4> register_passed_float_args = {Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3}; 

    //RegisterBitSet callee_saved_registers = RegisterBitSet({Register::B, Register::DI, Register::SI, Register::R12, Register::R13, Register::R14, Register::R15});
    //RegisterBitSet caller_saved_registers = RegisterBitSet({Register::A, Register::C, Register::D, Register::R8, Register::R9, Register::R10});

#endif

#ifdef __linux__
    std::array<Register, 6> register_passed_int_args = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};
    std::array<Register, 8> register_passed_float_args = {Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3, 
                                                        Register::XMM4, Register::XMM5, Register::XMM6, Register::XMM7};

    RegisterBitSet callee_saved_registers = RegisterBitSet({
        Register::B,
        Register::R15,
        Register::R14,
        Register::R13,
        Register::R12});
    RegisterBitSet caller_saved_registers = RegisterBitSet({
        Register::A,  
        Register::C,
        Register::D,
        Register::SI,
        Register::DI,
        Register::R8,
        Register::R9,
        Register::R10,
        Register::R11,
        Register::XMM0,
        Register::XMM1,
        Register::XMM2,
        Register::XMM3,
        Register::XMM4,
        Register::XMM5,
        Register::XMM6,
        Register::XMM7,
        Register::XMM8,
        Register::XMM9,
        Register::XMM10,
        Register::XMM11,
        Register::XMM12,
        Register::XMM13,
        Register::XMM14,
        Register::XMM15
        });
#endif
};

extern RegisterData g_register_data;
}

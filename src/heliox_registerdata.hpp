#pragma once
#include <array> 
#include <set>
#include <string>
#include <cstdint>
#include "heliox_error.hpp"

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

inline bool is_gp_register(Register reg)
{
    return reg >= Register::A && reg <= Register::R15;
}

inline bool is_xmm_register(Register reg)
{
    return reg >= Register::XMM0 && reg <= Register::XMM15;
}

inline Register get_register_from_string(const std::string& reg)
{
    if (reg == "rax")
        return Register::A;
    else if (reg == "rbx")
        return Register::B;
    else if (reg == "rcx")
        return Register::C;
    else if (reg == "rdx")
        return Register::D;
    else if (reg == "rsi")
        return Register::SI;
    else if (reg == "rdi")
        return Register::DI;
    else if (reg == "rbp")
        return Register::BP;
    else if (reg == "rsp")
        return Register::SP;
    else if (reg == "r8")
        return Register::R8;
    else if (reg == "r9")
        return Register::R9;
    else if (reg == "r10")
        return Register::R10;
    else if (reg == "r11")
        return Register::R11;
    else if (reg == "r12")
        return Register::R12;
    else if (reg == "r13")
        return Register::R13;
    else if (reg == "r14")
        return Register::R14;
    else if (reg == "r15")
        return Register::R15;
    else if (reg == "xmm0") 
        return Register::XMM0;
    else if (reg == "xmm1") 
        return Register::XMM1;
    else if (reg == "xmm2") 
        return Register::XMM2;
    else if (reg == "xmm3") 
        return Register::XMM3;
    else if (reg == "xmm4") 
        return Register::XMM4;
    else if (reg == "xmm5") 
        return Register::XMM5;
    else if (reg == "xmm6") 
        return Register::XMM6;
    else if (reg == "xmm7") 
        return Register::XMM7;
    else if (reg == "xmm8") 
        return Register::XMM8;
    else if (reg == "xmm9") 
        return Register::XMM9;
    else if (reg == "xmm10") 
        return Register::XMM10;
    else if (reg == "xmm11") 
        return Register::XMM11;
    else if (reg == "xmm12") 
        return Register::XMM12;
    else if (reg == "xmm13") 
        return Register::XMM13;
    else if (reg == "xmm14") 
        return Register::XMM14;
    else if (reg == "xmm15") 
        return Register::XMM15;
    Logger::not_implemented();
}

inline std::string get_register(Register reg, uint32_t byte_size)
{
    switch (reg)
    {
        case Register::XMM0:
            return "xmm0";
        case Register::XMM1:
            return "xmm1";
        case Register::XMM2:
            return "xmm2";
        case Register::XMM3:
            return "xmm3";
        case Register::XMM4:
            return "xmm4";
        case Register::XMM5:
            return "xmm5";
        case Register::XMM6:
            return "xmm6";
        case Register::XMM7:
            return "xmm7";
        case Register::XMM8:
            return "xmm8";
        case Register::XMM9:
            return "xmm9";
        case Register::XMM10:
            return "xmm10";
        case Register::XMM11:
            return "xmm11";
        case Register::XMM12:
            return "xmm12";
        case Register::XMM13:
            return "xmm13";
        case Register::XMM14:
            return "xmm14";
        case Register::XMM15:
            return "xmm15";
        default:
            break;
    }
    switch (byte_size) 
    {
    case 8:
        switch (reg)
        {
        case Register::A:
            return "rax";
        case Register::B:
            return "rbx";
        case Register::C:
            return "rcx";
        case Register::D:
            return "rdx";
        case Register::SP:
            return "rsp";
        case Register::BP:
            return "rbp";
        case Register::SI:
            return "rsi";
        case Register::DI:
            return "rdi";
        case Register::R8:
            return "r8";
        case Register::R9:
            return "r9";
        case Register::R10:
            return "r10";
        case Register::R11:
            return "r11";
        case Register::R12:
            return "r12";
        case Register::R13:
            return "r13";
        case Register::R14:
            return "r14";
        case Register::R15:
            return "r15";
        default:
            Logger::not_implemented();
        }
    case 4:
        switch (reg)
        {
        case Register::A:
            return "eax";
        case Register::B:
            return "ebx";
        case Register::C:
            return "ecx";
        case Register::D:
            return "edx";
        case Register::SP:
            return "esp";
        case Register::BP:
            return "ebp";
        case Register::SI:
            return "esi";
        case Register::DI:
            return "edi";
        case Register::R8:
            return "r8d";
        case Register::R9:
            return "r9d";
        case Register::R10:
            return "r10d";
        case Register::R11:
            return "r11d";
        case Register::R12:
            return "r12d";
        case Register::R13:
            return "r13d";
        case Register::R14:
            return "r14d";
        case Register::R15:
            return "r15d";
        default:
            Logger::not_implemented();
        }
    case 2:
        switch (reg)
        {
        case Register::A:
            return "ax";
        case Register::B:
            return "bx";
        case Register::C:
            return "cx";
        case Register::D:
            return "dx";
        case Register::SP:
            return "sp";
        case Register::BP:
            return "bp";
        case Register::SI:
            return "si";
        case Register::DI:
            return "di";
        case Register::R8:
            return "r8w";
        case Register::R9:
            return "r9w";
        case Register::R10:
            return "r10w";
        case Register::R11:
            return "r11w";
        case Register::R12:
            return "r12w";
        case Register::R13:
            return "r13w";
        case Register::R14:
            return "r14w";
        case Register::R15:
            return "r15w";
        default:
            Logger::not_implemented();
        }
    case 1:
        switch (reg)
        {
        case Register::A:
            return "al";
        case Register::B:
            return "bl";
        case Register::C:
            return "cl";
        case Register::D:
            return "dl";
        case Register::SP:
            return "spl";
        case Register::BP:
            return "bpl";
        case Register::SI:
            return "sil";
        case Register::DI:
            return "dil";
        case Register::R8:
            return "r8b";
        case Register::R9:
            return "r9b";
        case Register::R10:
            return "r10b";
        case Register::R11:
            return "r11b";
        case Register::R12:
            return "r12b";
        case Register::R13:
            return "r13b";
        case Register::R14:
            return "r14b";
        case Register::R15:
            return "r15b";
        default:
            Logger::not_implemented();
        }
    default:
        Logger::error("", HX_ILLEGAL_REG_SIZE, std::format("tried to get a register of size: {} ({})", byte_size, (int)reg));
    }
}

struct RegisterData
{
    RegisterData() 
    : available_registers([this]
        {
            std::set<Register> s = available_general_purpose_registers;
            s.insert(available_xmm_registers.begin(), available_xmm_registers.end());
            return s;
        }())
    {}
    // R11 SCRATCH REGISTER 
    //std::set<Register> available_general_purpose_registers = {Register::A, Register::B, Register::C, Register::R15, Register::R14};
    const std::set<Register> available_general_purpose_registers = {Register::A, Register::B, Register::C,
            Register::D, Register::DI, Register::SI, Register::R8, Register::R9, Register::R10,
            Register::R12, Register::R13, Register::R14, Register::R15};

    
    const std::set<Register> available_xmm_registers = { Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3, Register::XMM4, Register::XMM5,
            Register::XMM6, Register::XMM7, Register::XMM8, Register::XMM9, Register::XMM10,
            Register::XMM12, Register::XMM13, Register::XMM14, Register::XMM15};
    
    const std::set<Register> available_registers;


    const Register gp_scratch_register = Register::R11;
    const Register xmm_scratch_register = Register::XMM11;
    
#ifdef _WIN32

    const std::array<Register, 4> register_passed_int_args = {Register::C, Register::D, Register::R8, Register::R9};
    const std::array<Register, 4> register_passed_float_args = {Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3}; 

    const std::set<Register> callee_saved_registers = {Register::B, Register::DI, Register::SI, Register::R12, Register::R13, Register::R14, Register::R15, 
        Register::XMM6, Register::XMM7, Register::XMM8, Register::XMM9, Register::XMM10, Register::XMM11, Register::XMM12, Register::XMM13, Register::XMM14, Register::XMM15};
    const std::set<Register> caller_saved_registers = {Register::A, Register::C, Register::D, Register::R8, Register::R9, Register::R10, 
        Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3, Register::XMM4, Register::XMM5};

#endif

#ifdef __linux__
    const std::array<Register, 6> register_passed_int_args = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};
    const std::array<Register, 8> register_passed_float_args = {Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3, 
                                                        Register::XMM4, Register::XMM5, Register::XMM6, Register::XMM7};

    const std::set<Register> callee_saved_registers = {
        Register::B,
        Register::R15,
        Register::R14,
        Register::R13,
        Register::R12};
    const std::set<Register> caller_saved_registers = {
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
        Register::XMM12,
        Register::XMM13,
        Register::XMM14,
        Register::XMM15
        };
#endif
};

extern RegisterData g_register_data;


} // namespace hx

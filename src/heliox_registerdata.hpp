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

inline std::string get_register(Register reg, uint32_t byte_size)
{
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
        }
    default:
        Logger::error("", HX_ILLEGAL_REG_SIZE, std::format("tried to get a register of size: {}", byte_size));
    }
}

struct RegisterData
{
    // R11 SCRATCH REGISTER 
    std::set<Register> available_general_purpose_registers = {Register::A, Register::B};
    //std::set<Register> available_general_purpose_registers = {Register::A, Register::B, Register::C,
    //        Register::D, Register::DI, Register::SI, Register::R8, Register::R9, Register::R10,
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

    std::set<Register> callee_saved_registers = {Register::B, Register::DI, Register::SI, Register::R12, Register::R13, Register::R14, Register::R15};
    std::set<Register> caller_saved_registers = {Register::A, Register::C, Register::D, Register::R8, Register::R9, Register::R10};

#endif

#ifdef __linux__
    std::array<Register, 6> register_passed_int_args = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};
    std::array<Register, 8> register_passed_float_args = {Register::XMM0, Register::XMM1, Register::XMM2, Register::XMM3, 
                                                        Register::XMM4, Register::XMM5, Register::XMM6, Register::XMM7};

    //RegisterBitSet callee_saved_registers = RegisterBitSet({
    //    Register::B,
    //    Register::R15,
    //    Register::R14,
    //    Register::R13,
    //    Register::R12});
    //RegisterBitSet caller_saved_registers = RegisterBitSet({
    //    Register::A,  
    //    Register::C,
    //    Register::D,
    //    Register::SI,
    //    Register::DI,
    //    Register::R8,
    //    Register::R9,
    //    Register::R10,
    //    Register::R11,
    //    Register::XMM0,
    //    Register::XMM1,
    //    Register::XMM2,
    //    Register::XMM3,
    //    Register::XMM4,
    //    Register::XMM5,
    //    Register::XMM6,
    //    Register::XMM7,
    //    Register::XMM8,
    //    Register::XMM9,
    //    Register::XMM10,
    //    Register::XMM11,
    //    Register::XMM12,
    //    Register::XMM13,
    //    Register::XMM14,
    //    Register::XMM15
    //    });
#endif
};

extern RegisterData g_register_data;


} // namespace hx

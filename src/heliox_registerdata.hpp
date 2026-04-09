#pragma once
#include <print>
#include <array> 
#include <vector>
#include <bitset>
#include <utility>

namespace hx {

enum class Register: int
{
    NOREG=-1, A, B, C, D, SP, BP, SI, DI, R8, R9, R10, R11, R12, R13, R14, R15
};

enum class RegisterSize
{
    BIT0,
    BIT8,
    BIT16,
    BIT32,
    BIT64
};


inline std::string register_size_to_prefix(RegisterSize reg_size)
{
    switch (reg_size)
    {
        case RegisterSize::BIT64:
            return "qword";
        case RegisterSize::BIT32:
            return "dword";
        case RegisterSize::BIT16:
            return "word";
        case RegisterSize::BIT8:
            return "byte";
        case RegisterSize::BIT0:
            std::println("ERROR: TRIED TO GET REGISTER SIZE 0 PREFIX");
            exit(-1);
    }
}

inline std::string register_to_string(Register reg, RegisterSize reg_size)
{
    switch (reg_size)
    {
        case RegisterSize::BIT64:
            switch (reg)
            {
                case Register::A:   return "rax";
                case Register::B:   return "rbx";
                case Register::C:   return "rcx";
                case Register::D:   return "rdx";
                case Register::SP:  return "rsp";
                case Register::BP:  return "rbp";
                case Register::SI:  return "rsi";
                case Register::DI:  return "rdi";
                case Register::R8:  return "r8"; 
                case Register::R9:  return "r9"; 
                case Register::R10: return "r10";
                case Register::R11: return "r11";
                case Register::R12: return "r12";
                case Register::R13: return "r13";
                case Register::R14: return "r14";
                case Register::R15: return "r15";
                default: return "NOT A REGISTER";
            }
        case RegisterSize::BIT32:
            switch (reg)
            {
                case Register::A:   return "eax";
                case Register::B:   return "ebx";
                case Register::C:   return "ecx";
                case Register::D:   return "edx";
                case Register::SP:  return "esp";
                case Register::BP:  return "ebp";
                case Register::SI:  return "esi";
                case Register::DI:  return "edi";
                case Register::R8:  return "r8d"; 
                case Register::R9:  return "r9d"; 
                case Register::R10: return "r10d";
                case Register::R11: return "r11d";
                case Register::R12: return "r12d";
                case Register::R13: return "r13d";
                case Register::R14: return "r14d";
                case Register::R15: return "r15d";
                default: return "NOT A REGISTER";
            }
        case RegisterSize::BIT16:
            switch (reg)
            {
                case Register::A:   return "ax";
                case Register::B:   return "bx";
                case Register::C:   return "cx";
                case Register::D:   return "dx";
                case Register::SP:  return "sp";
                case Register::BP:  return "bp";
                case Register::SI:  return "si";
                case Register::DI:  return "di";
                case Register::R8:  return "r8w"; 
                case Register::R9:  return "r9w"; 
                case Register::R10: return "r10w";
                case Register::R11: return "r11w";
                case Register::R12: return "r12w";
                case Register::R13: return "r13w";
                case Register::R14: return "r14w";
                case Register::R15: return "r15w";
                default: return "NOT A REGISTER";
            }
        case RegisterSize::BIT8:
            switch (reg)
            {
                case Register::A:   return "al";
                case Register::B:   return "bl";
                case Register::C:   return "cl";
                case Register::D:   return "dl";
                case Register::SP:  return "spl";
                case Register::BP:  return "bpl";
                case Register::SI:  return "sil";
                case Register::DI:  return "dil";
                case Register::R8:  return "r8b"; 
                case Register::R9:  return "r9b"; 
                case Register::R10: return "r10b";
                case Register::R11: return "r11b";
                case Register::R12: return "r12b";
                case Register::R13: return "r13b";
                case Register::R14: return "r14b";
                case Register::R15: return "r15b";
                default: return "NOT A REGISTER";
            }

        case RegisterSize::BIT0:
            std::println("ERROR: TRIED TO GET REGISTER SIZE REGISTER PREFIX");
            exit(-1);
    }
}

inline uint32_t get_byte_size_from_register_size(RegisterSize reg_size)
{
    switch (reg_size)
    {
        case RegisterSize::BIT64:
            return 8; 
        case RegisterSize::BIT32:
            return 4; 
        case RegisterSize::BIT16:
            return 2; 
        case RegisterSize::BIT8:
            return 1; 
    }

}

inline RegisterSize get_register_size(uint32_t byte_size)
{
    switch (byte_size)
    {
        case 8:
            return RegisterSize::BIT64;
        case 4:
            return RegisterSize::BIT32;
        case 2:
            return RegisterSize::BIT16;
        case 1:
            return RegisterSize::BIT8;
        case 0:
            return RegisterSize::BIT0;
        default:
            //TODO ERROR
            std::println("Tried to fetch virtual register from invalid size: '{}'", byte_size);
            exit(-1);

    }
    
}

struct RegisterBitSet {
    static const size_t register_count = 16;
    RegisterBitSet() = default; 
    RegisterBitSet(const std::vector<Register>& bits)
    {
        for (auto reg : bits)
        {
            set(reg);
        }
    }
    RegisterBitSet(std::bitset<register_count> other_bits) 
    {
        bits = other_bits;   
    }
    std::bitset<register_count> bits;
    void set(Register b) { bits.set(std::to_underlying(b)); }
    void reset(Register b) { bits.reset(std::to_underlying(b)); }
    bool test(Register b) const { return bits.test(std::to_underlying(b)); }
    void flip() { bits.flip(); }
    RegisterBitSet get_bits_not_in_other(const RegisterBitSet& other) { return {bits & ~other.bits};}
    RegisterBitSet get_bits_in_other(const RegisterBitSet& other) { return {bits & other.bits};}
    

    size_t count() const { return bits.count(); }
    Register get_first_available() const 
    { 
        for (size_t i = 0; i < register_count; i++) 
        {
            if (bits.test(i)) return static_cast<Register>(i);
        } 
        return Register::NOREG; 
    }
    std::vector<Register> get_available_registers()
    {
        std::vector<Register> available_registers; 
        for (int i = 0; i < register_count; i++) 
        {
            if (bits.test(i)) available_registers.push_back(static_cast<Register>(i));
        } 
        return available_registers;
    }
};

struct RegisterData
{
    // R11 SCRATCH REGISTER 
    RegisterBitSet available_registers = RegisterBitSet({Register::A, Register::B, Register::C,
            Register::D, Register::DI, Register::SI, Register::R8, Register::R9, Register::R10,
            Register::R12, Register::R13, Register::R14, Register::R15});     

    std::array<Register, 6> register_passed_arguments = {Register::DI, Register::SI, Register::D, Register::C, Register::R8, Register::R9};


};

extern RegisterData g_register_data;
}

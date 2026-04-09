#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <unordered_map>
#include <print>

namespace hx {
enum class primitive_type
{
    VOID,
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    
};

inline std::unordered_map<std::string, primitive_type> primitive_type_map = 
{
    {"u8",   primitive_type::U8},
    {"u16",  primitive_type::U16},
    {"u32",  primitive_type::U32},
    {"u64",  primitive_type::U64},
    {"i8",   primitive_type::I8},
    {"i16",  primitive_type::I16},
    {"i32",  primitive_type::I32},
    {"i64",  primitive_type::I64},
    {"void", primitive_type::VOID}
 
};

constexpr uint32_t get_byte_size_from_known_type(primitive_type type)
{
    switch (type)
    {
        case primitive_type::U8:  return 1;
        case primitive_type::U16: return 2;
        case primitive_type::U32: return 4;
        case primitive_type::U64: return 8;

        case primitive_type::I8:  return 1;
        case primitive_type::I16: return 2;
        case primitive_type::I32: return 4;
        case primitive_type::I64: return 8;
        
        case primitive_type::VOID: return 0;
        default:
            // TODO ERROR
           std::println("Type not implemented");
           exit(-1);
   }     
}
constexpr uint32_t get_ptr_byte_size()
{
    return 8;
}

inline std::optional<primitive_type> get_primitive_type_from_string(const std::string& type_name)
{
    if (!primitive_type_map.count(type_name)) return std::nullopt;
    return {primitive_type_map.at(type_name)}; 
}


struct type_data
{
    type_data(primitive_type type, uint32_t ptr_depth)
        :
            type(type),
            ptr_depth(ptr_depth)
    {
        if (ptr_depth) 
            byte_size = get_ptr_byte_size();
        else
            byte_size = get_byte_size_from_known_type(type);
    }

    friend bool operator!=(const type_data& left, const type_data& right)
    {
        return (left.type != right.type) && (left.ptr_depth != right.ptr_depth);
    }

    friend bool operator==(const type_data& left, const type_data& right)
    {
        return (left.type == right.type) && (left.ptr_depth == right.ptr_depth);
    }

    type_data(primitive_type type)
        : type(type), ptr_depth(0)
    {
        byte_size = get_byte_size_from_known_type(type);
    }
   primitive_type type; 
   uint32_t byte_size;
   uint32_t ptr_depth;
    
};


inline bool is_unsigned(const type_data td)
{
    if (td.ptr_depth != 0) return true;
    switch (td.type)
    {
        case primitive_type::I8:
        case primitive_type::I16:
        case primitive_type::I32:
        case primitive_type::I64:
            return false;

        case primitive_type::U8:
        case primitive_type::U16:
        case primitive_type::U32:
        case primitive_type::U64:
            return true;
    default:
        std::println("ERROR: unknown type at function: is_unsigned");
        exit(-1);

    }
}

}

#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <unordered_map>
#include <print>
#include "heliox_error.hpp"

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
    F32,
    F64,
    USER_DEFINED_STRUCT
};

inline std::unordered_map<std::string_view, primitive_type> primitive_type_map = 
{
    {"u8",   primitive_type::U8},
    {"u16",  primitive_type::U16},
    {"u32",  primitive_type::U32},
    {"u64",  primitive_type::U64},
    {"i8",   primitive_type::I8},
    {"i16",  primitive_type::I16},
    {"i32",  primitive_type::I32},
    {"i64",  primitive_type::I64},
    {"f32",  primitive_type::F32},
    {"f64",  primitive_type::F64},
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
        
        case primitive_type::F32: return 4;
        case primitive_type::F64: return 8;

        case primitive_type::VOID: return 0;
        default:
            // TODO ERROR
            Logger::error("", HX_TODO, "Type not implemented");
   }     
}
constexpr uint32_t get_ptr_byte_size()
{
    return 8;
}

inline primitive_type get_primitive_type_from_string(std::string_view type_name)
{
    if (!primitive_type_map.count(type_name)) return primitive_type::USER_DEFINED_STRUCT;
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
        {
            if (type == primitive_type::USER_DEFINED_STRUCT)
            {
                Logger::error("", HX_TODO, "Size of user defined struct is not known at this point");
            }
            else
            byte_size = get_byte_size_from_known_type(type);
        }
    }

    type_data(const type_data& other)
        : type(other.type), ptr_depth(other.ptr_depth), byte_size(other.byte_size)
    { }


    type_data deref() const
    {
        if (ptr_depth == 0)
        {
            Logger::error("", HX_ILLEGAL_DEREF, "Cannot dereference non-pointer type");
        }
        return type_data(type, ptr_depth - 1);
    }

    type_data get_ptr_type() const
    {
        return type_data(type, ptr_depth + 1);
    }

    friend bool operator!=(const type_data& left, const type_data& right)
    {
        return !(left == right);
    }

    friend bool operator==(const type_data& left, const type_data& right)
    {
        return (left.type == right.type) && (left.ptr_depth == right.ptr_depth);
    }

   primitive_type type; 
   uint32_t byte_size;
   uint32_t ptr_depth;
};

inline bool is_integer_type(const type_data td) 
{
    if (td.ptr_depth != 0) return true;
    switch (td.type)
    {
        case primitive_type::I8:
        case primitive_type::I16:
        case primitive_type::I32:
        case primitive_type::I64:
        case primitive_type::U8:
        case primitive_type::U16:
        case primitive_type::U32:
        case primitive_type::U64:
            return true;
        default:
            return false;
    }
}

inline bool is_float_type(const type_data td) 
{
    if (td.ptr_depth != 0) return false;
    switch (td.type)
    {
        case primitive_type::F32:
        case primitive_type::F64:
            return true;
        default:
            return false;
    }
}

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

inline bool is_implicit_conversion_possible(const type_data t1, const type_data t2)
{
    if (is_integer_type(t1) && is_integer_type(t2))
        return true;

    if (is_float_type(t1) && is_float_type(t2))
        return true;

    return false;
}

}

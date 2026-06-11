#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include "typedefs.hpp"
#include "heliox_error.hpp"

namespace hx {


struct Scope;

enum struct PrimitiveType
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
};

struct StructType 
{
    Scope* scope;
};

using UnresolvedType = std::string;

using BaseType = std::variant<UnresolvedType, PrimitiveType, StructType>;


struct Type
{
    static Type Unresolved(const UnresolvedType& name, uint32_t ptr_depth) { return Type{name, ptr_depth}; }
    static Type Primitive(PrimitiveType basic, uint32_t ptr_depth) { return Type{basic, ptr_depth}; }

    BaseType base;
    uint32_t ptr_depth;

    /* TODO */
    uint32_t byte_size() const
    {
        return std::visit(
        overloads{
            [](PrimitiveType primitive_type) 
            {
            switch (primitive_type)
            {
                case PrimitiveType::U8:  return 1;
                case PrimitiveType::U16: return 2;
                case PrimitiveType::U32: return 4;
                case PrimitiveType::U64: return 8;

                case PrimitiveType::I8:  return 1;
                case PrimitiveType::I16: return 2;
                case PrimitiveType::I32: return 4;
                case PrimitiveType::I64: return 8;

                case PrimitiveType::F32: return 4;
                case PrimitiveType::F64: return 8;

                case PrimitiveType::VOID: return 0;
            }
            },
            []([[maybe_unused]]const StructType& struct_type)
            {
                Logger::not_implemented();
                return 0;
            },
            [] (const UnresolvedType& unresolved_type){
                Logger::error("", std::format("unresolved type: {}", unresolved_type)); 
                return 0; 
            }
            },
            base);
    }
    
    friend bool operator == (const Type& a, const Type& b)
    {
        if (a.base.index() != b.base.index())
            return false;

        return std::visit(
            overloads{
            [&a, &b](const PrimitiveType at)
            {
                const PrimitiveType bt = std::get<PrimitiveType>(b.base);
                return (at == bt) && (a.ptr_depth == b.ptr_depth);
            },
            [](auto&&)
            {
            // TODO
            Logger::not_implemented();
            return false;
            }
            },
            a.base);
    }
    /* TODO */

    std::optional<Type> deref() const
    {
        if (ptr_depth == 0)
        {
            return std::nullopt;
        }
        return Type(base, ptr_depth - 1);
    }
    Type get_ptr() const
    {
        return Type(base, ptr_depth + 1);
    }
};


inline constexpr Type TYPE_F32  = Type(PrimitiveType::F32,  0);
inline constexpr Type TYPE_F64  = Type(PrimitiveType::F64,  0);
inline constexpr Type TYPE_I8   = Type(PrimitiveType::I8,   0);
inline constexpr Type TYPE_I16  = Type(PrimitiveType::I16,  0);
inline constexpr Type TYPE_I32  = Type(PrimitiveType::I32,  0);
inline constexpr Type TYPE_I64  = Type(PrimitiveType::I64,  0);
inline constexpr Type TYPE_U8   = Type(PrimitiveType::U8,   0);
inline constexpr Type TYPE_U16  = Type(PrimitiveType::U16,  0);
inline constexpr Type TYPE_U32  = Type(PrimitiveType::U32,  0);
inline constexpr Type TYPE_U64  = Type(PrimitiveType::U64,  0);
inline constexpr Type TYPE_VOID = Type(PrimitiveType::VOID, 0);


/* TODO */
inline bool is_float_type(const Type& t)
{
    if (t.ptr_depth != 0) return false;     
    if (!std::holds_alternative<PrimitiveType>(t.base))
    {
        return false;
    }

    PrimitiveType pt = std::get<PrimitiveType>(t.base);
    
    switch (pt)
    {
        case PrimitiveType::F32:
        case PrimitiveType::F64:
            return true;
        default:
            return false;
    }

}

inline bool is_integer_type(const Type& t)
{
    if (t.ptr_depth != 0) return true;
    if (!std::holds_alternative<PrimitiveType>(t.base))
    {
        return false;
    }

    PrimitiveType pt = std::get<PrimitiveType>(t.base);
    
    switch (pt)
    {
        case PrimitiveType::I8:
        case PrimitiveType::I16:
        case PrimitiveType::I32:
        case PrimitiveType::I64:
        case PrimitiveType::U8:
        case PrimitiveType::U16:
        case PrimitiveType::U32:
        case PrimitiveType::U64:
            return true;
        default:
            return false;
    }

}

inline bool is_implicit_conversion_possible(const Type& t1, const Type& t2)
{
    if (is_integer_type(t1) && is_integer_type(t2)) return true;
    if (is_float_type(t1) && is_float_type(t2)) return true;

    return false;
}
/* TODO */

inline std::unordered_map<std::string_view, PrimitiveType> primitive_type_map = 
{
    {"u8",   PrimitiveType::U8},
    {"u16",  PrimitiveType::U16},
    {"u32",  PrimitiveType::U32},
    {"u64",  PrimitiveType::U64},
    {"i8",   PrimitiveType::I8},
    {"i16",  PrimitiveType::I16},
    {"i32",  PrimitiveType::I32},
    {"i64",  PrimitiveType::I64},
    {"f32",  PrimitiveType::F32},
    {"f64",  PrimitiveType::F64},
    {"void", PrimitiveType::VOID}
 
};



/*
constexpr uint32_t get_ptr_byte_size()
{
    return 8;
}

inline primitive_type get_primitive_type_from_string(std::string_view type_name)
{
    if (!primitive_type_map.count(type_name)) return primitive_type::USER_DEFINED_STRUCT;
    return primitive_type_map.at(type_name); 
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
   uint32_t ptr_depth;
   uint32_t byte_size;
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
*/
}

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <variant>
#include "typedefs.hpp"
#include "heliox_error.hpp"

namespace hx {

inline int64_t align_up(int64_t offset, int64_t align)
{
    return (offset + align - 1) & ~(align - 1);
}

struct Type;

struct UserDefinedStruct;
inline UserDefinedStruct& get_user_defined_struct(size_t id);

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

struct StructType {
    StructType(size_t id, uint32_t byte_size) : id(id), byte_size(byte_size) {}
    size_t id;
    uint32_t byte_size;
};

struct ArrayType {
    sptr<Type> underlying_type;
    uint32_t element_count;
};

using UnresolvedType = std::string;
using BaseType = std::variant<UnresolvedType, PrimitiveType, StructType, ArrayType>;


struct Type
{
    Type(BaseType base, uint32_t ptr_depth, std::vector<uint32_t> _array_element_counts) 
        : base(base), ptr_depth(ptr_depth), array_element_counts(_array_element_counts) {}
    Type(BaseType base, uint32_t ptr_depth) 
        : base(base), ptr_depth(ptr_depth) {}

    static Type Unresolved(const UnresolvedType& name, uint32_t ptr_depth, const std::vector<uint32_t>& _array_element_counts) { 
        return Type{name, ptr_depth, _array_element_counts}; 
    }

    static Type Primitive(PrimitiveType basic, uint32_t ptr_depth) { 
        return Type{basic, ptr_depth};
    }

    static Type Struct(StructType st) {
        return Type{st, 0};
    }
    static Type Array(sptr<Type> underlying, uint32_t element_count) {
        return Type(ArrayType(underlying, element_count), 0);
    }

    BaseType base;
    uint32_t ptr_depth;
    uint32_t offset = 0;

    std::vector<uint32_t> array_element_counts = {};

public:

    bool is_array() const {
        return std::holds_alternative<ArrayType>(base);
    }

    void set_struct_byte_size(uint32_t byte_size) {
        if (ptr_depth > 0) Logger::internal_error();
        if (!std::holds_alternative<StructType>(base)) Logger::internal_error();
        StructType& st = std::get<StructType>(base);
        st.byte_size = byte_size;
    }

    
    uint32_t base_type_byte_size() const {
        return std::visit(
        overloads{
            [](PrimitiveType primitive_type) -> uint32_t 
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
                default:
                    Logger::internal_error();
            }
            },
            [] (const UnresolvedType& unresolved_type) -> uint32_t {
                Logger::error("", std::format("unresolved type: {}", unresolved_type)); 
                return 0; 
            },
            [] (const StructType& struct_type) -> uint32_t {
                return 8;
            },
            [] (const ArrayType& array_type) -> uint32_t {
                return 8;
            }
            },
            base);
    }

    uint32_t byte_size() const
    {
        if (ptr_depth != 0) return 8;
        return base_type_byte_size();
    }

    uint32_t actual_byte_size() const 
    {
        if (ptr_depth != 0) return 8;
        
        return std::visit(
        overloads{
            [this](PrimitiveType primitive_type) -> uint32_t  {
            {
                return byte_size(); 
            }
            },
            [] (const UnresolvedType& unresolved_type) -> uint32_t {
                Logger::error("", std::format("unresolved type: {}", unresolved_type)); 
                return 0; 
            },
            [] (const StructType& struct_type) -> uint32_t {
                return struct_type.byte_size;
            },
            [this] (const ArrayType& array_type) -> uint32_t {
                return array_byte_size();
            }
            },
            base);
    }

    uint32_t array_byte_size() const 
    {
        if (!std::holds_alternative<ArrayType>(base) || ptr_depth != 0) Logger::internal_error(); 
        const ArrayType& array_type = std::get<ArrayType>(base);
        if (array_type.underlying_type->is_array()) 
            return array_type.underlying_type->array_byte_size() * array_type.element_count;
        return array_type.underlying_type->byte_size() * array_type.element_count;
    }

    uint32_t struct_byte_size() const 
    {
        if (!std::holds_alternative<StructType>(base) || ptr_depth != 0) Logger::internal_error(); 
        const StructType& struct_type = std::get<StructType>(base);
        return struct_type.byte_size;
    }
    
    
    friend bool operator == (const Type& a, const Type& b)
    {
        if (a.ptr_depth != b.ptr_depth) 
            return false;

        if (a.base.index() != b.base.index())
            return false;

        return std::visit(
            overloads{
            [&a, &b](const PrimitiveType at)
            {
                const PrimitiveType bt = std::get<PrimitiveType>(b.base);
                return at == bt;
            },
            [&a, &b](const StructType at)
            {
                const StructType bt = std::get<StructType>(b.base);
                return at.id == bt.id;
                
            },
            [&a, &b](const ArrayType at)
            {
                const ArrayType bt = std::get<ArrayType>(b.base);
                return at.underlying_type == bt.underlying_type;                
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
            if (is_array()) {
                return *std::get<ArrayType>(base).underlying_type;
            }
            return std::nullopt;
        }
        return Type(base, ptr_depth - 1);
    }
    Type get_ptr() const
    {
        return Type(base, ptr_depth + 1);
    }

};


struct UserDefinedStruct {
    UserDefinedStruct()=default;

    void calculate_offset() {
        uint32_t offset = 0;
        for (auto& [name, type] : fields) {
            uint32_t size = type.byte_size();
            type.offset = (uint32_t)align_up((int64_t)offset, (int64_t)size);
            offset = type.offset + size;
            
            struct_alignment = std::max(size, struct_alignment);
        }
        byte_size = (uint32_t)align_up((int64_t)offset, (int64_t)struct_alignment);
    }

    std::map<std::string, Type> fields;
    uint32_t struct_alignment = 0;
    uint32_t byte_size = 0;
};

inline std::vector<UserDefinedStruct>& get_user_defined_structs() {
    static std::vector<UserDefinedStruct> user_defined_structs;
    return user_defined_structs;
}

inline UserDefinedStruct& get_user_defined_struct(size_t id) {
    return get_user_defined_structs()[id];
}

inline StructType push_user_defined_struct() {
    auto& user_defined_structs = get_user_defined_structs();
    user_defined_structs.push_back({});
    return StructType{user_defined_structs.size() - 1ul, 0};
}


inline const Type TYPE_F32  = Type(PrimitiveType::F32,  0);
inline const Type TYPE_F64  = Type(PrimitiveType::F64,  0);
inline const Type TYPE_I8   = Type(PrimitiveType::I8,   0);
inline const Type TYPE_I16  = Type(PrimitiveType::I16,  0);
inline const Type TYPE_I32  = Type(PrimitiveType::I32,  0);
inline const Type TYPE_I64  = Type(PrimitiveType::I64,  0);
inline const Type TYPE_U8   = Type(PrimitiveType::U8,   0);
inline const Type TYPE_U16  = Type(PrimitiveType::U16,  0);
inline const Type TYPE_U32  = Type(PrimitiveType::U32,  0);
inline const Type TYPE_U64  = Type(PrimitiveType::U64,  0);
inline const Type TYPE_VOID = Type(PrimitiveType::VOID, 0);


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

inline bool is_struct_type(const Type& t) 
{
    if (t.ptr_depth != 0) return false;     
    return std::holds_alternative<StructType>(t.base);
}

// naming kinda sus 
inline bool is_string(const Type& t) {
    if (!std::holds_alternative<PrimitiveType>(t.base))
    {
        return false;
    }
    PrimitiveType pt = std::get<PrimitiveType>(t.base);
    return (t.ptr_depth == 1 && pt == PrimitiveType::U8);
}

inline bool is_pointer_type(const Type& t) {
    return t.ptr_depth > 0;
}

inline bool is_array_type(const Type& t) {
    if (t.ptr_depth > 0) return false;
    return std::holds_alternative<ArrayType>(t.base);
}


inline bool is_integer_type(const Type& t)
{
    if (t.ptr_depth != 0) return true;
    if (is_array_type(t)) return true; // arrays act like pointers
    //if (is_struct_type(t)) return true; // structs act like pointers
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

/*
TODO use this maybe:
inline bool is_integer_representable_type(const Type& t)
{
    if (t.ptr_depth != 0) return true;
    if (is_array_type(t)) return true; // arrays act like pointers
    if (is_struct_type(t)) return true; // structs act like pointers
    
    return is_integer_type(t);
}
*/

inline bool is_unsigned(const Type& t)
{
    if (t.ptr_depth != 0) return false;
    if (!std::holds_alternative<PrimitiveType>(t.base))
    {
        return false;
    }

    PrimitiveType pt = std::get<PrimitiveType>(t.base);
    
    switch (pt)
    {
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
    if (is_array_type(t1) && is_integer_type(t2)) return true;
    if (is_integer_type(t1) && is_array_type(t2)) return true;
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

} // namespace hx


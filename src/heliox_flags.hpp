#pragma once
#include <map>
#include <string>


using hx_flags = std::map<std::string, bool>;
inline hx_flags& HX_GET_FLAGS()
{
    static hx_flags flags;
    return flags;
}

inline bool HX_IS_FLAG(const std::string& flag_name) 
{
    const hx_flags& flags = HX_GET_FLAGS();
    return flags.count(flag_name);
}

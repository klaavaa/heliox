#pragma once
#include <string>
#include <cstdint>

namespace hx
{


struct ast_node
{
    ast_node(std::string_view filename, uint32_t line, uint32_t position)
        : filename(filename), line(line), position(position) {}
    std::string_view filename;   
    uint32_t line;
    uint32_t position;
};

} // namespace hx
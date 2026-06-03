#pragma once
#include <string_view>
#include <string>
#include <print>
#include "heliox_error.hpp"

namespace hx
{

struct CompileFlags
{
    bool compile_only = false;
    bool compile_and_assemble_only = false;
    std::string output_file = "";

};

CompileFlags& get_compile_flags()
{
    static CompileFlags flags;
    return flags;
}

inline void process_flag(std::string_view flag_string, int* i, int argc, char** argv)
{
#ifdef _WIN32
    const std::string_view program_name = "heliox.exe";
#else
    const std::string_view program_name = "heliox";
#endif

    CompileFlags& flags = get_compile_flags();

    constexpr auto format = "  {:<24} {}";


    if (flag_string == "-h" || flag_string == "--help")
    {
        std::println("Usage: {} [options] <files>", program_name);
        std::println("Options:");
        std::println(format, "--help", "Display this information");
        std::println(format, "-o <file>", "Place the output into <file>");
        std::println(format, "-c <file>", "Compile and assemble, but do not link");
        std::println(format, "-S <file>", "Compile only; do not assemble or link");

        exit(0);
    }

    if (flag_string == "-c")
    {
        flags.compile_and_assemble_only = true;
        return;
    }

    if (flag_string == "-S")
    {
        flags.compile_only = true;
        return;
    }

    if (flag_string == "-o")
    {
        if (*i + 1 == argc) Logger::pre_compile_error("'-o' expects an output file");
        *i += 1;
        flags.output_file = std::string(argv[*i]);
        return;
    }

    Logger::pre_compile_error("unregognized command-line option: '{}'", flag_string);
}



} // namespace hx
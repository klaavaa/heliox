#include <iostream>
#include <map>
#include <string>
#include <print>
#include "heliox_timer.hpp"
#include "heliox_flags.hpp"
#include "heliox_compile.hpp"
#include "heliox_symbol_table.hpp"
#if !defined(_WIN32) && !defined(__linux__)
#error "Unsupported platform"
#endif


int main(int argc, char** argv)
{
     
    hx_flags& flags = HX_GET_FLAGS();

    std::vector<std::string> file_paths;//= { "../example.hlx" };
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            flags[argv[i]] = true;
            continue;
        }
        file_paths.emplace_back(argv[i]);
    }

    if (file_paths.empty())
    {
        std::println("Please specify the files you want to compile");
        return 0;
    }
    
    auto func = fn<void, const std::vector<std::string>&, const std::string&>(hx::compile);
    double time = timeit<void, const std::vector<std::string>&, const std::string&>(func, file_paths, "./");
    std::println("Compile time: {}s", time);


    return 0;
}

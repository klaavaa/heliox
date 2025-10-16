#include <iostream>
#include <map>
#include <string>
#include "heliox_timer.hpp"
#include "heliox_flags.hpp"
#include "heliox_compile.hpp"

int main(int argc, char** argv)
{

    if (argc <= 1)
        return 0;

    hx_flags& flags = HX_GET_FLAGS();

    std::string file_path = ""; 
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            flags[argv[i]] = true;
            continue;
        }
        file_path = argv[i];
    }

    if (file_path.empty())
    {
        std::cout << "Please specify the file you want to compile" << "\n";
        return 0;
    }
    
    auto func = fn<void, const std::string&, const std::string&>(hx_compile);
    double time = hx_timeit<void, const std::string&, const std::string&>(func, file_path, "./");
    std::cout << "Compile time: " << time << "s\n";


    return 0;
}

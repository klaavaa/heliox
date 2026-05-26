
#pragma once
#include <vector>
#include <unordered_set>
#include "heliox_function.hpp"
#include "heliox_pointer.hpp"

namespace hx
{

    struct Module : public std::enable_shared_from_this<Module>
    {
        Module(std::string name="", sptr<Module> parent_module = nullptr)
            : name(name), parent_module(parent_module) {}

        std::vector<uptr<function>> functions;
        std::string name;

        sptr<Module> parent_module;
        std::unordered_map<std::string, sptr<Module>> submodules;

        void insert_function(uptr<function> func)
        {
            functions.push_back(std::move(func));
        }

        std::string get_full_name() const
        {
            if (parent_module)
            {
                return parent_module->get_full_name() + "::" + name;
            }
            return name;
        }
        
        std::vector<std::string> get_module_path() const
        {
            if (name.empty())
            {
                return {};
            }

            std::vector<std::string> path;
            if (parent_module)
            {
                path = parent_module->get_module_path();
            }
            path.push_back(name);
            return path;
        }

        sptr<Module> find_submodule(std::vector<std::string>& module_path, size_t index = 0)
        {
            if (index >= module_path.size())
            {
                Logger::error("", HX_MODULE_NOT_FOUND, "Module not found");
            }
            if (!submodules.contains(module_path[index]))
            {
                Logger::error("", HX_MODULE_NOT_FOUND, "Module not found");
            }
            else
            {
                if (index == module_path.size() - 1)
                {
                    return submodules.at(module_path[index]);
                }
                return submodules.at(module_path[index])->find_submodule(module_path, index + 1);        
            }
        }


    };

    inline sptr<Module> create_or_get_submodule(sptr<Module> parent, std::string name)
    {
        if (parent->submodules.contains(name))
        {
            return parent->submodules.at(name);
        }
        sptr<Module> submodule = std::make_shared<Module>(name, parent);
        parent->submodules.insert({name, submodule});
        return submodule;
    }

    
    inline void merge_modules(sptr<Module> parent, sptr<Module> to, sptr<Module> from)
    {
        to->functions.insert(to->functions.end(), std::make_move_iterator(from->functions.begin()), std::make_move_iterator(from->functions.end()));

        for (const auto& [name, submodule] : from->submodules)
        {
            if (to->submodules.contains(name))
            {
                merge_modules(to, to->submodules.at(name), submodule);
            }
            else
            {
                to->submodules.insert({name, submodule});
            }
        }
    }


    struct TranslationUnit
    {
        TranslationUnit(sptr<Module> global_module, std::vector<uptr<import_statement>> imports, std::string_view filename)
            : global_module(std::move(global_module)), imports(std::move(imports)), filename(filename) {}
        
        sptr<Module> global_module;
        std::vector<uptr<import_statement>> imports;

        std::string_view filename;
    };

    struct Program 
    {
        Program(const std::vector<uptr<TranslationUnit>>& translation_units)
        : global_module(std::make_shared<Module>())
        {
           for (auto& tu : translation_units)
           {
               for (auto& [name, module] : tu->global_module->submodules)
               {
                    if (global_module->submodules.contains(name))
                    {
                        merge_modules(nullptr, global_module->submodules.at(name), module);
                    }

                    else
                    {
                        global_module->submodules.insert({name, module});
                    }
               }
           }
        }

        sptr<Module> global_module;
    };

}


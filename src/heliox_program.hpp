
#pragma once
#include <vector>
#include "typedefs.hpp"
#include "heliox_statement.hpp"
#include "heliox_symbol_table.hpp"

namespace hx
{

    struct TranslationUnit
    {
        TranslationUnit(std::string_view filename, std::vector<statement>& _statements)
            : filename(filename), statements(std::move(_statements)) {}
        
        std::string_view filename;
        std::vector<statement> statements;

        sptr<Scope> global_scope;

    };

    struct Program 
    {
        Program(std::vector<TranslationUnit>& _translation_units)
            :
            translation_units(_translation_units)
        {
            // program scope contains basic types 
            program_scope = create_program_scope();
            

            for (auto& tu: translation_units)   
            {
                tu.global_scope = program_scope->get_child();
            }

        }

        std::vector<TranslationUnit>& translation_units;
        sptr<Scope> program_scope;
    };

}


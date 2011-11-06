/*--------------------------------------------------------------------
(C) Copyright 2006-2009 Barcelona Supercomputing Center 
Centro Nacional de Supercomputacion

This file is part of Mercurium C/C++ source-to-source compiler.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

Mercurium C/C++ source-to-source compiler is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public
License along with Mercurium C/C++ source-to-source compiler; if
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.
--------------------------------------------------------------------*/

#include "cxx-utils.h"
#include "tl-analysis-phase.hpp"
#include "tl-cfg-visitor.hpp"
#include "tl-static-analysis.hpp"

namespace TL
{
    AnalysisPhase::AnalysisPhase()
    {
        set_phase_name("Experimental phase for analysis");
        set_phase_description("This phase builds a Control Flow Graph and performs different analysis on demand. ");
    }
    
    void AnalysisPhase::run(TL::DTO& dto)
    {
        RefPtr<Nodecl::NodeclBase> nodecl = RefPtr<Nodecl::NodeclBase>::cast_dynamic(dto["nodecl"]);
        
        // *** Build the graphs for every method in the translation unit *** //
//         DEBUG_CODE()
        {
            std::cerr << std::endl << "=== CFG Construction ===" << std::endl;
        }        
        CfgVisitor cfg_visitor;
        cfg_visitor.build_cfg(nodecl, std::string(""));
        
        ObjectList<ExtensibleGraph*> cfgs = cfg_visitor.get_cfgs();
        
        // *** Global variables labelling *** //
        std::cerr << std::endl << "=== GLOBAL VARIABLES ANNOTATION ===" << std::endl;
        for (ObjectList<ExtensibleGraph*>::iterator it = cfgs.begin(); it != cfgs.end(); ++it)
        {
            std::cerr << std::endl << "   ==> Graph '" << (*it)->get_name() << "'" << std::endl;
            (*it)->compute_global_variables_usage();
            ObjectList<struct global_var_usage_t*> global_vars = (*it)->get_global_variables();
            for (ObjectList<struct global_var_usage_t*>::iterator its = global_vars.begin(); its != global_vars.end(); ++its)
            {
                std::cerr << "       - " << (*its)->get_nodecl().prettyprint() << std::endl;
            }
        }
       
        // *** Use-def chains + IPA *** //
        std::cerr << std::endl << "=== USE-DEF CHAINS COMPUTATION ===" << std::endl;
        for (ObjectList<ExtensibleGraph*>::iterator it = cfgs.begin(); it != cfgs.end(); ++it)
        {
            if (!(*it)->has_use_def_computed())
            {  
                (*it)->init_function_call_nest();
                cfg_visitor.set_last_func_call((*it)->get_function_call_nest());
                cfg_visitor.set_actual_cfg(*it);
                cfg_visitor.compute_use_def_chains((*it)->get_graph());
                (*it)->set_use_def_computed();
            }
        }
      
        // *** Live Variable Analysis *** //
        std::cerr << std::endl << "=== LIVE VARIABLES AND TASKS ANALYSIS  ===" << std::endl;
        for (ObjectList<ExtensibleGraph*>::iterator it = cfgs.begin(); it != cfgs.end(); ++it)
        {
//                 DEBUG_CODE()
            {
                std::cerr << std::endl << " ==> Graph '" << (*it)->get_name() << "'" << std::endl;
            }
                
            // Non-task nodes
            StaticAnalysis::live_variable_analysis((*it)->get_graph());
            
            // Task nodes
            StaticAnalysis::analyse_tasks((*it)->get_tasks_list());
        }
        
        // Print graphs into dot files
        for (ObjectList<ExtensibleGraph*>::iterator it = cfgs.begin(); it != cfgs.end(); ++it)
        {
            ExtensibleGraph::print_graph_to_dot((*it)->get_graph(), (*it)->get_name());
        }
    }
}

EXPORT_PHASE(TL::AnalysisPhase);

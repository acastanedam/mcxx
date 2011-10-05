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


#include "cxx-codegen.h"
#include "cxx-process.h"
#include "tl-node.hpp"


namespace TL
{
    Node::Node()
        : _id(-1), _entry_edges(), _exit_edges(), _visited(false)
    {
        set_data(_NODE_TYPE, UNCLASSIFIED_NODE);
    }
    
    Node::Node(int& id, Node_type ntype, Node* outer_graph)
        : _id(++id), _entry_edges(), _exit_edges(), _visited(false)
    {
        set_data(_NODE_TYPE, ntype);
        
        if (outer_graph != NULL)
        {
            set_data(_OUTER_NODE, outer_graph);
        }

        if (ntype == GRAPH_NODE)
        {
            set_data(_ENTRY_NODE, new Node(id, BASIC_ENTRY_NODE, NULL));
            int a = -2; set_data(_EXIT_NODE, new Node(a, BASIC_EXIT_NODE, NULL));
        }
    }
    
    Node::Node(int& id, Node_type type, Node* outer_graph, ObjectList<Nodecl::NodeclBase> nodecls)
        : _id(++id), _entry_edges(), _exit_edges(), _visited(false)
    {        
        set_data(_NODE_TYPE, type);
        
        if (outer_graph != NULL)
        {    
            set_data(_OUTER_NODE, outer_graph);
        }
        
        set_data(_NODE_STMTS, nodecls);
    }
    
    Node::Node(int& id, Node_type type, Node* outer_graph, Nodecl::NodeclBase nodecl)
        : _id(++id), _entry_edges(), _exit_edges(), _visited(false)
    {      
        set_data(_NODE_TYPE, type);
        
        if (outer_graph != NULL)
        {    
            set_data(_OUTER_NODE, outer_graph);
        }
        
        set_data(_NODE_STMTS, ObjectList<Nodecl::NodeclBase>(1,nodecl));
    }
    
    Node* Node::copy(Node* outer_node)
    {
        Node_type ntype = get_data<Node_type>(_NODE_TYPE);
        
        if (ntype == GRAPH_NODE)
        {
            internal_error("Method copy node can only be used with non GRAPH_NODE node types", 0);
        }
        
        // Create the node
        Node* new_node = new Node(_id, ntype, outer_node);
        
        // Set the special properties for each node
        switch(ntype)
        {
            case BASIC_LABELED_NODE:
            case BASIC_GOTO_NODE: 
                new_node->set_data(_NODE_LABEL, get_data<Symbol>(_NODE_LABEL));
            case BASIC_FUNCTION_CALL_NODE:
            case BASIC_NORMAL_NODE:
            {
                ObjectList<Nodecl::NodeclBase> stmts = get_data<ObjectList<Nodecl::NodeclBase> >(_NODE_STMTS);
                new_node->set_data(_NODE_STMTS, stmts);
                
                if (has_key(_LIVE_IN))
                {   // Liveness analysis has been performed, so we copy this information too
                    new_node->set_data(_LIVE_IN, get_data<ext_sym_set>(_LIVE_IN));
                    new_node->set_data(_LIVE_OUT, get_data<ext_sym_set>(_LIVE_OUT));
                    new_node->set_data(_UPPER_EXPOSED, get_data<ext_sym_set>(_UPPER_EXPOSED));
                    new_node->set_data(_KILLED, get_data<ext_sym_set>(_KILLED));
                }
                if (has_key(_IN_DEPS))
                {   // Auto-deps analysis has been performed
                    new_node->set_data(_IN_DEPS, get_data<ext_sym_set>(_IN_DEPS));
                    new_node->set_data(_OUT_DEPS, get_data<ext_sym_set>(_OUT_DEPS));
                    new_node->set_data(_INOUT_DEPS, get_data<ext_sym_set>(_INOUT_DEPS));                    
                }
                break;
            }
            case FLUSH_NODE:    // FIXME We have to set here the sybols to be flushed
                break;
            default:            // nothing to do
                break;
        }
        
        return new_node;
    }
    
    void Node::erase_entry_edge(Node* source)
    {
        ObjectList<Edge*>::iterator it;
        for (it = _entry_edges.begin(); 
                it != _entry_edges.end();
                ++it)
        {
            if ((*it)->get_source() == source)
            {
                _entry_edges.erase(it);
                --it;   // Decrement to allow the correctness of the comparison outside the loop
                break;
            }
        }
        if (it == _entry_edges.end())
        {
            std::cerr << " ** Node.cpp :: erase_entry_edge() ** "
                      << "Trying to delete an non-existent edge " 
                      << "between nodes '" << source->_id << "' and '" << _id << "'" << std::endl;
        }
    }
    
    void Node::erase_exit_edge(Node* target)
    {
        ObjectList<Edge*>::iterator it;
        for (it = _exit_edges.begin(); 
                it != _exit_edges.end();
                ++it)
        {
            if ((*it)->get_target() == target)
            {
                _exit_edges.erase(it);
                --it;   // Decrement to allow the correctness of the comparison outside the loop
                break;
            }
        }
        if (it == _exit_edges.end())
        {
            std::cerr << " ** Node.cpp :: exit_entry_edge() ** "
                      << "Trying to delete an non-existent edge " 
                      << "between nodes '" << _id << "' and '" << target->_id << "'" << std::endl;  
     
        }
    }
    
    int Node::get_id() const
    {
        return _id;
    }
    
    void Node::set_id(int id)
    {
        _id = id;
    }
    
    bool Node::is_visited() const
    {
        return _visited;
    }
    
    void Node::set_visited(bool visited)
    {
        _visited = visited;
    }
    
    bool Node::is_empty_node()
    {
        return (_id==-1 && get_data<Node_type>(_NODE_TYPE) == UNCLASSIFIED_NODE);
    }
    
    ObjectList<Edge*> Node::get_entry_edges() const
    {
        return _entry_edges;
    }
    
    void Node::set_entry_edge(Edge *entry_edge)
    {
        _entry_edges.append(entry_edge);
    }
    
    ObjectList<Edge_type> Node::get_entry_edge_types()
    {
        ObjectList<Edge_type> result;
        
        for(ObjectList<Edge*>::iterator it = _entry_edges.begin();
                it != _entry_edges.end();
                ++it)
        {
            result.append((*it)->get_data<Edge_type>(_EDGE_TYPE));
        }
        
        return result;
    }
    
    ObjectList<std::string> Node::get_entry_edge_labels()
    {
        ObjectList<std::string> result;
        
        for(ObjectList<Edge*>::iterator it = _entry_edges.begin();
                it != _entry_edges.end();
                ++it)
        {
            result.append((*it)->get_label());
        }
        
        return result;
    }
    
    ObjectList<Node*> Node::get_parents()
    {
        ObjectList<Node*> result;
        
        for(ObjectList<Edge*>::iterator it = _entry_edges.begin();
                it != _entry_edges.end();
                ++it)
        {
            result.append((*it)->get_source());
        }
        
        return result;
    }
    
    ObjectList<Edge*> Node::get_exit_edges() const
    {
        return _exit_edges;
    }
    
    void Node::set_exit_edge(Edge *exit_edge)
    {
        _exit_edges.append(exit_edge);
    }
    
    ObjectList<Edge_type> Node::get_exit_edge_types()
    {
        ObjectList<Edge_type> result;
        
        for(ObjectList<Edge*>::iterator it = _exit_edges.begin();
                it != _exit_edges.end();
                ++it)
        {
            result.append((*it)->get_data<Edge_type>(_EDGE_TYPE));
        }
        
        return result;
    }
    
    ObjectList<std::string> Node::get_exit_edge_labels()
    {
        ObjectList<std::string> result;
        
        for(ObjectList<Edge*>::iterator it = _exit_edges.begin();
                it != _exit_edges.end();
                ++it)
        {
            result.append((*it)->get_label());
        }
        
        return result;
    }
    
    Edge* Node::get_exit_edge(Node* target)
    {
        Edge* result = NULL;
        int id = target->get_id();
        for(ObjectList<Edge*>::iterator it = _exit_edges.begin();
            it != _exit_edges.end();
            ++it)
        {
            if ((*it)->get_target()->get_id() == id)
            { 
                result = *it;
                break;
            }
        }
        return result;
    }
    
    ObjectList<Node*> Node::get_children()
    {
        ObjectList<Node*> result;
        
        for(ObjectList<Edge*>::iterator it = _exit_edges.begin();
                it != _exit_edges.end();
                ++it)
        {
            result.append((*it)->get_target());
        }
        
        return result;
    }

    bool Node::is_basic_node()
    {
        if (has_key(_NODE_TYPE))
        {
            Node_type nt = get_data<Node_type>(_NODE_TYPE);
            return (nt != GRAPH_NODE);
        }
        else
        {
            std::cerr << " ** Node.cpp :: is_basic_node() ** "
                      << "warning: The node '" << _id << "' has not type." << std::endl;
            return false;
        }
    }

    bool Node::is_connected()
    {
        return (!_entry_edges.empty() || !_exit_edges.empty());
    }

    bool Node::has_child(Node* n)
    {
        bool result = false;
        int id = n->_id;
        
        for (ObjectList<Edge*>::iterator it = _exit_edges.begin(); it != _exit_edges.end(); ++it)
        {
            if ((*it)->get_target()->_id == id)
            {    
                result = true;
                break;
            }
        }
        
        return result;
    }

    bool Node::has_parent(Node* n)
    {
        bool result = false;
        int id = n->_id;
        
        for (ObjectList<Edge*>::iterator it = _entry_edges.begin(); it != _entry_edges.end(); ++it)
        {
            if ((*it)->get_source()->_id == id)
            {    
                result = true;
                break;
            }
        }
        
        return result;
    }

    bool Node::operator==(const Node* &n) const
    {
        return ((_id == n->_id) && (_entry_edges == n->_entry_edges) && (_exit_edges == n->_exit_edges));
    }

    ObjectList<Node*> Node::get_inner_nodes()
    {
        if (get_data<Node_type>(_NODE_TYPE) != GRAPH_NODE)
        {
            return ObjectList<Node*>();
        }
        else
        {
            ObjectList<Node*> node_l;
            get_inner_nodes_rec(node_l);
            
            return node_l;
        }
    }

    void Node::get_inner_nodes_rec(ObjectList<Node*>& node_l)
    {
        if (!_visited)
        {
            set_visited(true);
            
            Node* actual = this;
            Node_type ntype = get_data<Node_type>(_NODE_TYPE);
            if (ntype == GRAPH_NODE)
            {
                // Get the nodes inside the graph
                Node* entry = get_data<Node*>(_ENTRY_NODE);
                entry->set_visited(true);
                actual = entry;
            }
            else if (ntype == BASIC_EXIT_NODE)
            {
                return;
            }
            else if ((ntype == BASIC_NORMAL_NODE) || (ntype == BASIC_LABELED_NODE) || (ntype == BASIC_FUNCTION_CALL_NODE))
            {
                // Include the node into the list
                node_l.insert(this);
            }
            else if ((ntype == BASIC_GOTO_NODE) || (ntype == BASIC_BREAK_NODE) 
                     || (ntype == FLUSH_NODE) || (ntype == BARRIER_NODE) || (ntype == BASIC_PRAGMA_DIRECTIVE_NODE))
            {   // do nothing
            }
            else
            {
                internal_error("Unexpected kind of node '%s'", get_type_as_string().c_str());
            }
            
            ObjectList<Node*> next_nodes = actual->get_children();
            for (ObjectList<Node*>::iterator it = next_nodes.begin();
                it != next_nodes.end();
                ++it)
            {
                (*it)->get_inner_nodes_rec(node_l);
            }            
        }
        
        return;
    }

    Symbol Node::get_function_node_symbol()
    {
       
        if (get_data<Node_type>(_NODE_TYPE) != BASIC_FUNCTION_CALL_NODE)
        {
            return Symbol();
        }
        
        Nodecl::NodeclBase stmt = get_data<ObjectList<Nodecl::NodeclBase> >(_NODE_STMTS)[0];
        Symbol s;
        if (stmt.is<Nodecl::FunctionCall>())
        {
            Nodecl::FunctionCall f = stmt.as<Nodecl::FunctionCall>();
            s = f.get_called().get_symbol();
        }
        else if (stmt.is<Nodecl::VirtualFunctionCall>())
        {
            Nodecl::FunctionCall f = stmt.as<Nodecl::FunctionCall>();
            s = f.get_called().get_symbol();
        }
        
        return s;
    }
    
    Node_type Node::get_type()
    {
        if (has_key(_NODE_TYPE))
            return get_data<Node_type>(_NODE_TYPE);
        else
            return UNCLASSIFIED_NODE;
    }

    std::string Node::get_type_as_string()
    {
        std::string type = "";
        if (has_key(_NODE_TYPE))
        {
            Node_type ntype = get_data<Node_type>(_NODE_TYPE);
            switch(ntype)
            {
                case BASIC_ENTRY_NODE:              type = "BASIC_ENTRY_NODE";
                break;
                case BASIC_EXIT_NODE:               type = "BASIC_EXIT_NODE";
                break;
                case BASIC_NORMAL_NODE:             type = "BASIC_NORMAL_NODE";
                break;
                case BASIC_LABELED_NODE:            type = "BASIC_LABELED_NODE";
                break;
                case BASIC_BREAK_NODE:              type = "BASIC_BREAK_NODE";
                break;
                case BASIC_CONTINUE_NODE:           type = "BASIC_CONTINUE_NODE";
                break;
                case BASIC_GOTO_NODE:               type = "BASIC_GOTO_NODE";
                break;
                case BASIC_FUNCTION_CALL_NODE:      type = "BASIC_FUNCTION_CALL_NODE";
                break;
                case BASIC_PRAGMA_DIRECTIVE_NODE:   type = "BASIC_PRAGMA_DIRECTIVE_NODE";
                break;
                case FLUSH_NODE:                    type = "FLUSH_NODE";
                break;
                case BARRIER_NODE:                  type = "BARRIER_NODE";
                break;
                case GRAPH_NODE:                    type = "GRAPH_NODE";
                break;
                case UNCLASSIFIED_NODE:             type = "UNCLASSIFIED_NODE";
                break;
                default: internal_error("Unexpected type of node '%s'", ntype);
            };
        }
        else
        {
            std::cerr << " ** Node.cpp :: is_basic_node() ** "
                      << "warning: The node '" << _id << "' has not type." << std::endl;
        }
        
        return type;
    }

    Node* Node::get_graph_entry_node()
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            return get_data<Node*>(_ENTRY_NODE);
        }
        else
        {
            internal_error("Unexpected node type '%s' while getting the entry node to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
    }

    void Node::set_graph_entry_node(Node* node)
    {
        if (node->get_data<Node_type>(_NODE_TYPE) != BASIC_ENTRY_NODE)
        {
            internal_error("Unexpected node type '%s' while setting the entry node to node '%s'. BASIC_ENTRY_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
        else if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            return set_data<Node*>(_ENTRY_NODE, node);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting the entry node to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
    }

    Node* Node::get_graph_exit_node()
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            return get_data<Node*>(_EXIT_NODE);
        }
        else
        {
            internal_error("Unexpected node type '%s' while getting the exit node to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
    }

    void Node::set_graph_exit_node(Node* node)
    {
        if (node->get_data<Node_type>(_NODE_TYPE) != BASIC_EXIT_NODE)
        {
            internal_error("Unexpected node type '%s' while setting the exit node to node '%s'. BASIC_EXIT_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
        else if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            return set_data<Node*>(_EXIT_NODE, node);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting the exit node to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
    }

    Node* Node::get_outer_node()
    {
        Node* outer_node = NULL;
        if (has_key(_OUTER_NODE))
        {
            outer_node = get_data<Node*>(_OUTER_NODE);
        }
        return outer_node;
    }
    
    void Node::set_outer_node(Node* node)
    {
        if (node->get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {    
            set_data(_OUTER_NODE, node);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting the exit node to node '%s'. GRAPH_NODE expected.",
                           node->get_type_as_string().c_str(), _id);
        }
    }

    ObjectList<Nodecl::NodeclBase> Node::get_statements()
    {
        ObjectList<Nodecl::NodeclBase> stmts;
        if (has_key(_NODE_STMTS))
        {
            stmts = get_data<ObjectList<Nodecl::NodeclBase> >(_NODE_STMTS);
        }
        return stmts;
    }

    void Node::set_statements(ObjectList<Nodecl::NodeclBase> stmts)
    {
        Node_type ntype = get_data<Node_type>(_NODE_TYPE);
        if (ntype == BASIC_NORMAL_NODE || ntype == BASIC_FUNCTION_CALL_NODE || ntype == BASIC_LABELED_NODE)
        {
            set_data(_NODE_STMTS, stmts);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting the statements to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);
        }
    }

    Nodecl::NodeclBase Node::get_graph_label(Nodecl::NodeclBase n)
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            Nodecl::NodeclBase res = Nodecl::NodeclBase::null();
            if (has_key(_NODE_LABEL))
            {
                res = get_data<Nodecl::NodeclBase>(_NODE_LABEL, n);
            }
            return res;
        }
        else
        {
            internal_error("Unexpected node type '%s' while getting the label to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);            
        }
    }
    
    void Node::set_graph_label(Nodecl::NodeclBase n)
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            set_data(_NODE_LABEL, n);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting the label to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);            
        }
    }

    std::string Node::get_graph_type()
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            return get_data<std::string>(_GRAPH_TYPE);
        }
        else
        {
            internal_error("Unexpected node type '%s' while getting graph type to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);            
        }
    }
           
    void Node::set_graph_type(std::string graph_type)
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            set_data(_GRAPH_TYPE, graph_type);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting graph type to node '%s'. GRAPH_NODE expected.",
                           get_type_as_string().c_str(), _id);            
        }
    }

    Symbol Node::get_label()
    {
        Node_type ntype = get_data<Node_type>(_NODE_TYPE);
        if (ntype == BASIC_GOTO_NODE || ntype == BASIC_LABELED_NODE)
        {
            return get_data<Symbol>(_NODE_LABEL);
        }
        else
        {
            internal_error("Unexpected node type '%s' while getting the label to node '%s'. GOTO or LABELED NODES expected.",
                           get_type_as_string().c_str(), _id);            
        }  
    }
           
    void Node::set_label(Symbol s)
    {
        Node_type ntype = get_data<Node_type>(_NODE_TYPE);
        if (ntype == BASIC_GOTO_NODE || ntype == BASIC_LABELED_NODE)
        {
            set_data(_NODE_LABEL, s);
        }
        else
        {
            internal_error("Unexpected node type '%s' while setting the label to node '%s'. GOTO or LABELED NODES expected.",
                           get_type_as_string().c_str(), _id);            
        }          
    }

    Node* Node::advance_over_non_statement_nodes()
    {
        ObjectList<Node*> children = get_children();
        Node_type ntype;
        Node* result = this;
        
        while ( (children.size() == 1) && (!result->has_key(_NODE_STMTS)) )
        {
            Node* c0 = children[0];
            ntype = c0->get_data<Node_type>(_NODE_TYPE);
            if (ntype == GRAPH_NODE)
            {
                result = c0->get_data<Node*>(_ENTRY_NODE);
            }
            else if (ntype == BASIC_EXIT_NODE)
            {
                if (c0->has_key(_OUTER_NODE))
                {
                    result = c0->get_data<Node*>(_OUTER_NODE);
                }
                else
                {    
                    break;
                }
            }
            else
            {
                result = c0;
            }
            children = result->get_children();
        }
       
        return result;
    }

    Node* Node::back_over_non_statement_nodes()
    {
        ObjectList<Node*> parents = get_parents();
        Node_type ntype;
        Node* result = this;
        
        while ( (parents.size() == 1) && (!result->has_key(_NODE_STMTS)) )
        {
            Node* p0 = parents[0];
            ntype = p0->get_data<Node_type>(_NODE_TYPE);
            
            if (ntype == GRAPH_NODE)
            {
                result = p0->get_data<Node*>(_EXIT_NODE);
            }
            else if (ntype == BASIC_ENTRY_NODE)
            {
                if (p0->has_key(_OUTER_NODE))
                {
                    result = p0->get_data<Node*>(_OUTER_NODE);
                }
                else
                {    
                    break;
                }
            }
            else
            {
                result = p0;
            }
            parents = result->get_parents();
        }
       
        return result;
    }
   
    ObjectList<ext_sym_set> Node::get_use_def_over_nodes()
    {
        ObjectList<ext_sym_set> use_def, use_def_aux;
        
        if (!_visited)          
        {
            _visited = true;
            ext_sym_set ue_vars, killed_vars, ue_aux, killed_aux;
           
            // Compute upper exposed variables
            ue_aux = get_ue_vars();
            for(ext_sym_set::iterator it = ue_aux.begin(); it != ue_aux.end(); ++it)
            {
                if (killed_vars.find(*it) == killed_vars.end())
                {
                    ue_vars.insert(*it);
                }
            }
            // Compute killed variables
            killed_aux = get_killed_vars();
            killed_vars.insert(killed_aux.begin(), killed_aux.end());
            
            // Complete the use-def info for every children of the node
            ObjectList<Node*> children = get_children();
            for(ObjectList<Node*>::iterator it = children.begin(); it != children.end(); ++it)
            {
                use_def_aux = (*it)->get_use_def_over_nodes();
                
                if (!use_def_aux.empty())
                {
                    // Compute upper exposed variables
                    ue_aux = use_def_aux[0];
                    for(ext_sym_set::iterator it = ue_aux.begin(); it != ue_aux.end(); ++it)
                    {
                        if (killed_vars.find(*it) == killed_vars.end())
                        {
                            ue_vars.insert(*it);
                        }
                    }
                    // Compute killed variables
                    killed_aux = use_def_aux[1];
                    killed_vars.insert(killed_aux.begin(), killed_aux.end());
                }
            }
            
            use_def.append(ue_vars); use_def.append(killed_vars);
        }
        
        return use_def;
    }
   
    void Node::set_graph_node_use_def()
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            _visited = true;
            Node* entry_node = get_data<Node*>(_ENTRY_NODE);
            
            // Get upper_exposed and killed variables
            ObjectList<ext_sym_set> use_def = entry_node->get_use_def_over_nodes();
            
            set_data(_UPPER_EXPOSED, use_def[0]);
            set_data(_KILLED, use_def[1]);
        }
        else
        {
            internal_error("Getting inner use-def information from node '%d' with type '%s' while "
                            "here it is mandatory a Graph node.\n",
                           _id, get_type_as_string().c_str());
        }
    }

    ext_sym_set Node::get_live_in_over_nodes()
    {
        ext_sym_set live_in_vars;
        
        // Advance over non-statements nodes while the path is unique
        Node* actual = advance_over_non_statement_nodes();
        
        // Get the live_in variables
        if (actual->has_key(_NODE_STMTS))
        {
            live_in_vars = actual->get_live_in_vars();
        }
        else
        {   // Node has more than one child
            ObjectList<Node*> actual_children = actual->get_children();
            ext_sym_set live_in_aux;
            Node_type ntype;
            
            for(ObjectList<Node*>::iterator it = actual_children.begin();
                it != actual_children.end();
                ++it)
            {
                // Advance over non-statements nodes while the path is unique
                ntype = (*it)->get_data<Node_type>(_NODE_TYPE);
                if (ntype == GRAPH_NODE)
                {
                    actual = (*it)->get_data<Node*>(_ENTRY_NODE);
                }
                else
                {    
                    actual = (*it);
                }
                actual = actual->advance_over_non_statement_nodes();
                
                live_in_aux = actual->get_live_in_vars();
                live_in_vars.insert(live_in_aux.begin(), live_in_aux.end());
            }
        }
        
        return live_in_vars;
    }
    
    ext_sym_set Node::get_live_out_over_nodes()
    {
        ext_sym_set live_out_vars;

        // Back over non-statements nodes while the path is unique
        Node* actual = back_over_non_statement_nodes();
        
        // Get the live_out variables
        if (actual->has_key(_NODE_STMTS))
        {
            live_out_vars = actual->get_live_out_vars();
        }
        else
        {   // Node has more than one parent
            ObjectList<Node*> actual_parents = actual->get_parents();
            
            ext_sym_set live_out_aux;
            for(ObjectList<Node*>::iterator it = actual_parents.begin();
                it != actual_parents.end();
                ++it)
            {
                Node_type ntype = (*it)->get_data<Node_type>(_NODE_TYPE);
                if (ntype == GRAPH_NODE)
                {
                    actual = (*it)->get_data<Node*>(_EXIT_NODE);
                }
                else
                {    
                    actual = (*it);
                }
                actual = actual->back_over_non_statement_nodes();
                
                live_out_aux = actual->get_live_out_vars();
                live_out_vars.insert(live_out_aux.begin(), live_out_aux.end());
            }
        }
        
        return live_out_vars;
    }
    
    void Node::set_graph_node_liveness()
    {
        if (get_data<Node_type>(_NODE_TYPE) == GRAPH_NODE)
        {
            // Get live in variables from entry node children
            Node* entry_node = get_data<Node*>(_ENTRY_NODE);
            set_data(_LIVE_IN, entry_node->get_live_in_over_nodes());
            
            // Get live out variables from exit node parents
            Node* exit_node = get_data<Node*>(_EXIT_NODE);
            set_data(_LIVE_OUT, exit_node->get_live_out_over_nodes());
        }
        else
        {
            internal_error("Getting inner liveness analysis from node '%d' with type '%s' while "
                            "here it is mandatory a Graph node.\n",
                           _id, get_type_as_string().c_str());
        }
    }
    
    ext_sym_set Node::get_live_in_vars()
    {
        ext_sym_set live_in_vars;
        
        if (has_key(_LIVE_IN))
        {
            live_in_vars = get_data<ext_sym_set>(_LIVE_IN);
        }

        return live_in_vars;
    }
    
    void Node::set_live_in(ExtensibleSymbol new_live_in_var)
    {
        ext_sym_set live_in_vars;
        
        if (has_key(_LIVE_IN))
        {
            live_in_vars = get_data<ext_sym_set>(_LIVE_IN);
        }        
        live_in_vars.insert(new_live_in_var);

        set_data(_LIVE_IN, live_in_vars);
    }
    
    void Node::set_live_in(ext_sym_set new_live_in_set)
    {
        set_data(_LIVE_IN, new_live_in_set);
    }
    
    ext_sym_set Node::get_live_out_vars()
    {
        ext_sym_set live_out_vars;
        
        if (has_key(_LIVE_OUT))
        {
            live_out_vars = get_data<ext_sym_set>(_LIVE_OUT);
        }
        
        return live_out_vars;
    }
    
    void Node::set_live_out(ExtensibleSymbol new_live_out_var)
    {
        ext_sym_set live_out_vars;
        
        if (has_key(_LIVE_OUT))
        {
            live_out_vars = get_data<ext_sym_set>(_LIVE_OUT);
        }
        live_out_vars.insert(new_live_out_var);
        
        set_data(_LIVE_OUT, live_out_vars);
    }
    
    void Node::set_live_out(ext_sym_set new_live_out_set)
    {
        set_data(_LIVE_OUT, new_live_out_set);
    }
    
    ext_sym_set Node::get_ue_vars()
    {
        ext_sym_set ue_vars;
        
        if (has_key(_UPPER_EXPOSED))
        {
            ue_vars = get_data<ext_sym_set>(_UPPER_EXPOSED);
        }
        
        return ue_vars;
    }
    
    void Node::set_ue_var(ExtensibleSymbol new_ue_var)
    {
        ext_sym_set ue_vars;
        
        std::cerr << "inserting UE var " << c_cxx_codegen_to_str(new_ue_var.get_nodecl().get_internal_nodecl()) << std::endl;
        
        if (this->has_key(_UPPER_EXPOSED))
        {   
            ue_vars = get_data<ext_sym_set>(_UPPER_EXPOSED);
            std::cerr << "       size before: " << ue_vars.size() << std::endl;
        }
        ue_vars.insert(new_ue_var);
        std::cerr << "       size after: " << ue_vars.size() << std::endl;
        
        set_data(_UPPER_EXPOSED, ue_vars);
    }
    
    ext_sym_set Node::get_killed_vars()
    {
        ext_sym_set killed_vars;
        
        if (has_key(_KILLED))
        {
            killed_vars = get_data<ext_sym_set>(_KILLED);
        }
        
        return killed_vars;
    }
    
    void Node::set_killed_var(ExtensibleSymbol new_killed_var)
    {
        ext_sym_set killed_vars;
        
        if (has_key(_KILLED))
        {
            killed_vars = get_data<ext_sym_set>(_KILLED);
        }
        killed_vars.insert(new_killed_var);
        
        set_data(_KILLED, killed_vars);
    }
    
    ext_sym_set Node::get_input_deps()
    {
        ext_sym_set input_deps;
        
        if (has_key(_IN_DEPS))
        {
            input_deps = get_data<ext_sym_set>(_IN_DEPS);
        }
        
        return input_deps;
    }    

    ext_sym_set Node::get_output_deps()
    {
        ext_sym_set output_deps;
        
        if (has_key(_OUT_DEPS))
        {
            output_deps = get_data<ext_sym_set>(_OUT_DEPS);
        }
        
        return output_deps;
    }  
    
    ext_sym_set Node::get_inout_deps()
    {
        ext_sym_set inout_deps;
        
        if (has_key(_INOUT_DEPS))
        {
            inout_deps = get_data<ext_sym_set>(_INOUT_DEPS);
        }
        
        return inout_deps;
    }      
}
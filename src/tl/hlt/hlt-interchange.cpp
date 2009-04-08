#include "hlt-interchange.hpp"
#include "hlt-exception.hpp"
#include <algorithm>

using namespace TL::HLT;

// This functor mimics sgi's iota
struct IotaGenerator
{
    private:
        int _n;
    public:
        IotaGenerator(int n)
            : _n(n) { }

        int operator()()
        {
            return (_n++);
        }
};

LoopInterchange::LoopInterchange(ForStatement for_stmt, ObjectList<int> permutation)
     : _for_nest(for_stmt), _permutation(permutation), _is_identity(false)
{
    // We could do sinking to achieve perfection
    if (!_for_nest.is_perfect())
    {
        // _ostream << for_stmt.get_ast().get_locus() << ": warning: interchange can only be applied in perfect loop nests";
        set_identity(for_stmt.prettyprint());
    }

    int nest_size = _for_nest.get_nest_list().size();
    if (!is_valid_permutation(_permutation, _is_identity)
            || (nest_size < _permutation.size()))
    {
        // _ostream << for_stmt.get_ast().get_locus() << ": warning: invalid permutation specification";
        // throw HLTException(for_stmt, 
        //         "invalid permutation specification");

        set_identity(for_stmt.prettyprint());
    }

    // Complete the permutation list if needed
    if (_permutation.size() < nest_size)
    {
        // Get the maximum
        int next = 1; 

        if (!_permutation.empty())
        {
            next = *(std::max_element(_permutation.begin(), _permutation.end())) + 1;
        }

        std::generate_n(back_inserter(_permutation), 
                nest_size - _permutation.size(),
                IotaGenerator(next));
    }
}

TL::Source LoopInterchange::get_source()
{
    return do_interchange();
}

TL::Source LoopInterchange::do_interchange()
{
    Source result;

    Source* current = new Source();

    result
        << (*current)
        ;

    ObjectList<ForStatement> loop_nest_list = _for_nest.get_nest_list();

    for (ObjectList<int>::iterator it = _permutation.begin();
            it != _permutation.end();
            it++)
    {
        ForStatement& current_for_stmt = loop_nest_list[(*it) - 1];

        Source* inner = new Source();

        (*current)
            << "for("
            << current_for_stmt.get_iterating_init().prettyprint() 
            << current_for_stmt.get_iterating_condition() << ";"
            << current_for_stmt.get_iterating_expression()
            << ")"
            << (*inner)
            ;

        // This is not wrong, every current is referenced in another current or in result
        delete current;
        current = inner;
    }

    // Now add the innermost loop body
    ForStatement& innermost_stmt = loop_nest_list[loop_nest_list.size() - 1];
    (*current)
        << innermost_stmt.get_loop_body()
        ;

    // See above why this is not wrong
    delete current;
    return result;
}

bool LoopInterchange::is_valid_permutation(ObjectList<int> permutation, bool &identity)
{
    identity = false;

    // Create a range
    ObjectList<int> range;
    std::generate_n(std::back_inserter(range), permutation.size(), IotaGenerator(1));

    if (std::equal(range.begin(), range.end(), permutation.begin()))
    {
        identity = true;
        return true;
    }

    std::sort(permutation.begin(), permutation.end());

    return std::equal(range.begin(), range.end(), permutation.begin());
}

LoopInterchange TL::HLT::loop_interchange(ForStatement for_stmt, ObjectList<int> permutation)
{
    return LoopInterchange(for_stmt, permutation);
}

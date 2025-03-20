#include "rpqdb/Graph.hpp"
#include "rpqdb/NFA.hpp"

namespace rpqdb {
    using namespace std;

    NFA query(NFA & data_nfa, const string& pattern) {
        // cout << "Data nfa" << endl;
        // data_nfa.print();
        NFA query_nfa = post2nfa(re2post(pattern)).getDFA();
        // cout << "Query NFA" << endl;
        // query_nfa.print();
        return query_nfa.product(data_nfa);
    }
}
// Support different query semantics
// PG algorithm, return the product graph

#include "rpqdb/Graph.hpp"
#include "rpqdb/NFA.hpp"
#include "unordered_map"
#include "unordered_set"

namespace rpqdb {
    using namespace std;

    // output-sensitive transitive closure
    unordered_map<int, unordered_set<int>> ostc(Graph & graph) {
        unordered_map<int, unordered_set<int>> E;
        unordered_map<int, unordered_set<int>> T;

        unordered_map<int, unordered_set<int>> T_prev;
        unordered_map<int, unordered_set<int>> delta_prev;
        
        auto negate_T_prev = [](const unordered_map<int, unordered_set<int>>& T_prev, int x, int y) -> bool {
            auto search = T_prev.find(x); 
            if (search == T_prev.end()) {
                return true; 
            } else {
                auto ans = search->second;
                if (ans.find(y) == ans.end()){
                    return true;
                } else {
                    return false;
                }
            }
        };

        for (const auto& [src, edges] : graph.adjList) {
            unordered_set<int> dst_set;
            for (const Edge& e : edges) {
                dst_set.insert(e.dest); 
            }
            E[src] = dst_set;
        }
        
        // delta T_0 and T_0
        delta_prev = E;
        T_prev = E;
        
        while (size(delta_prev) > 0) {
            unordered_map<int, unordered_set<int>> delta;
            // scan delta T^{i-1}
            for (const auto& [src, edges] : delta_prev) {
                // lookup for e in edges in E
                for (const auto& e: edges) {
                    auto ys = E[e];
                    for (const auto& y: ys) {
                        if (negate_T_prev(T_prev, src, y)) {
                            delta[src].insert(y);
                        }
                    }
                }
            }
            
            // T = T_prev + delta
            T = T_prev;
            for (const auto& [src, edges] : delta) {
                T[src].insert(edges.begin(), edges.end());
            }
            T_prev = T;
            delta_prev = delta;
        }

        return T;
    }

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

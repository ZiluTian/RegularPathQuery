#include "rpqdb/Graph.hpp"
#include "rpqdb/NFA.hpp"
#include "unordered_map"
#include "unordered_set"

namespace rpqdb {
    using namespace std;

    // PG with semi-naive evaluation
    // R(X, Y) = Ec(X, c, Y)
    // R(X, Z) = Eb(X, b, Y), R(Y, Z)
    // T(X, Z) = Ea(X, a, Y), R(Y, Z)
    // 
    // Initialization
    // delta R^0(X, Y) = Ec(X, c, Y)
    // R^0(X, Y) = delta R^0(X, Y)
    // T^0(X, Z) = Ea(X, a, Y) and R^0(Y, Z)
    // 
    // i = 0; repeat until delta T^i = \empty
    //  i += 1
    //  delta R^i(X, Z)  = Eb(X, b, Y) and delta R^{i-1}(Y, Z) and not R^{i-1}(X, Z)
    //  R^i(X, Y) = R^{i-1}(X, Y) or delta R^i(X, Y)
    //  delta T^i(X, Z)  = Ea(X, a, Y) and delta R^{i}(Y, Z) and not T^{i-1}(X, Z)
    //  T^i(X, Y) = T^{i-1}(X, Y) or delta T^i(X, Y)
    // return T^i
    ReachablePairs PG(Graph& product) {
        unordered_map<int, unordered_set<int>> Ea;
        unordered_map<int, unordered_set<int>> Eb_reverse; // fast lookup on the second column of Eb
        unordered_map<int, unordered_set<int>> Ec;
        unordered_map<int, unordered_set<int>> R;
        unordered_map<int, unordered_set<int>> T;

        unordered_map<int, unordered_set<int>> T_prev;
        unordered_map<int, unordered_set<int>> R_prev;
        unordered_map<int, unordered_set<int>> delta_R_prev;
        
        auto negate_prev = [](const unordered_map<int, unordered_set<int>>& prev, int x, int y) -> bool {
            auto search = prev.find(x); 
            if (search == prev.end()) {
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

        // Add self-loops corresponding to edges with label a
        for (const auto& vertex : product.starting_vertices) {
            Ea[vertex] = {vertex};
        }
        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex] = {vertex};
        }

        // All other edges correspond to edges with label b
        for (const auto& [src, edges] : product.adjList) {
            // unordered_set<int> dst_set;
            for (const Edge& e : edges) {
                // dst_set.insert(e.dest); 
                Eb_reverse[e.dest].insert(src);
            }
        }
        
        // ReachablePairs(Eb_reverse).print();

        // delta R_0 and R_0
        delta_R_prev = Ec;
        R_prev = Ec;
        
        // T^0 = Ea(X,a,Y), R^0(Y, Z)
        for (const auto& [x, ys] : Ea) {
            for (const auto& y: ys) {
                auto zs = R_prev[y];
                for (const auto& z: zs) {
                    T_prev[x].insert(z);                    
                }
            }    
        }

        // R is the only recursive relation. If no more delta_R relation can be derived, then there is also no more delta_T relation
        while (size(delta_R_prev) > 0) {
            unordered_map<int, unordered_set<int>> delta_R;
            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not R^{i-1}(X, Z)
            for (const auto& [y, zs] : delta_R_prev) {
                // lookup tuples in Eb
                for (const auto& z: zs) {
                    auto xs = Eb_reverse[y];
                    for (const auto& x: xs) {
                        if (negate_prev(R_prev, x, z)) {
                            delta_R[x].insert(z);
                        }
                    }
                }
            }
            
            // R^i(X, Y) = R^{i-1}(X, Y) or delta R^i(X, Y)
            R = R_prev;
            for (const auto& [src, edges] : delta_R) {
                R[src].insert(edges.begin(), edges.end());
            }
            R_prev = R;
            delta_R_prev = delta_R;

            //  delta T^i(X, Z) = Ea(X, a, Y) and delta R^{i}(Y, Z) and not T^{i-1}(X, Z)
            unordered_map<int, unordered_set<int>> delta_T;
            for (const auto& [x, ys] : Ea) {
                for (const auto& y: ys) {
                    auto zs = delta_R[y];
                    for (const auto& z: zs) {
                        if (negate_prev(T_prev, x, z)) {
                            delta_T[x].insert(z);
                        }
                    }
                }    
            }

            //  T^i(X, Y) = T^{i-1}(X, Y) or delta T^i(X, Y)
            T = T_prev;
            for (const auto& [src, edges] : delta_T) {
                T[src].insert(edges.begin(), edges.end());
            }
            T_prev = T;
        }

        return ReachablePairs(T);
    }

    // semi-naive / output-sensitive transitive closure
    // delta T^0(X, Y) = E(X, Y)
    // T^0(X, Y) = delta T^0(X, Y)
    // i = 0
    // repeat until delta T^i = \empty
    //  i += 1
    //  delta T^i(X, Y)  = delta T^{i-1}(X, Z) and E(Z, Y) and not T^{i-1}(X, Y)
    //  T^i(X, Y) = T^{i-1}(X, Y) or delta T^i(X, Y)
    // return T^i
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
                // lookup e in E
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
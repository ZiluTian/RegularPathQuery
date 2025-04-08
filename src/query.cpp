#include "unordered_map"
#include "unordered_set"
#include <cmath>
#include "rpqdb/Graph.hpp"
#include "rpqdb/NFA.hpp"
#include "rpqdb/Profiler.hpp"
#include <iterator>

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
    // 
    // i = 0; repeat until delta R^i = \empty
    //  i += 1
    //  delta R^i(X, Z)  = Eb(X, b, Y) and delta R^{i-1}(Y, Z) and not R^{i-1}(X, Z)
    //  R^i(X, Y) = R^{i-1}(X, Y) or delta R^i(X, Y)
    ReachablePairs PG(Graph&& product) {
        unordered_set<int> Ea;  // reflexive
        unordered_map<int, unordered_set<int>> Ec; 
        unordered_map<int, unordered_set<int>> R;
        unordered_map<int, unordered_set<int>> T;

        unordered_map<int, unordered_set<int>> R_prev;
        unordered_map<int, unordered_set<int>> delta_R_prev;
        
        START_LOCAL("PG semi-naive (Ea, Ec)");
        // Add self-loops corresponding to edges with label a
        Ea = std::move(product.starting_vertices);
        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex] = {vertex};
        }
        END_LOCAL();

        // delta R_0 and R_0
        START_LOCAL("PG semi-naive (delta_R0, R0, Eb_reverse)");
        delta_R_prev = Ec;
        R_prev = Ec;

        unordered_map<int, unordered_set<int>> Eb_reverse; // fast lookup on the second column of Eb
        if (!delta_R_prev.empty()) {
            // All other edges correspond to edges with label b
            for (const auto& [src, edges] : product.adjList) {
                // unordered_set<int> dst_set;
                for (const Edge& e : edges) {
                    // dst_set.insert(e.dest); 
                    Eb_reverse[e.dest].insert(src);
                }
            }
        }
        END_LOCAL();

        START_LOCAL("PG semi-naive (R)");
        unordered_map<int, unordered_set<int>> delta_R;
        while (!delta_R_prev.empty()) {
            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not R^{i-1}(X, Z)
            for (const auto& [y, zs] : delta_R_prev) {
                auto xs = Eb_reverse[y];
                for (const auto& x: xs) {
                    if (R_prev.find(x) == R_prev.end()) {
                        delta_R[x] = zs;
                    } else {
                        for (const auto& z: zs) {
                            if (R_prev[x].find(z) == R_prev[x].end()) {
                                delta_R[x].insert(z);
                            }
                        }
                    }
                }
            }
            
            // R^i(X, Y) = R^{i-1}(X, Y) or delta R^i(X, Y)
            for (const auto& [src, edges] : delta_R) {
                R_prev[src].insert(edges.begin(), edges.end());
            }
            delta_R_prev = std::move(delta_R);
            delta_R.clear();
        }
        R = std::move(R_prev);
        END_LOCAL();

        START_LOCAL("PG semi-naive (T)");
        // T(X, Z) = Ea(X, a, X), R(X, Z)
        for (const auto& [x, zs] : R) {
            if (Ea.find(x)!=Ea.end()){
                T[x].insert(zs.begin(), zs.end());
            }
        }
        END_LOCAL();
        return ReachablePairs(T);
    }

    ReachablePairs OSPG(Graph&& product) {
        // A bound for heavy/light partition of R
        int bound = std::floor(std::sqrt(product.getEdges()))+1;

        unordered_set<int> Ea;
        unordered_map<int, unordered_set<int>> Eb_reverse; // fast lookup on the second column of Eb
        unordered_map<int, unordered_set<int>> Ec;

        unordered_map<int, unordered_set<int>> R;
        unordered_map<int, unordered_set<int>> R_prev;
        unordered_map<int, unordered_set<int>> delta_R_prev;

        unordered_map<int, unordered_set<int>> R_light;
        unordered_set<int> R_heavy;

        unordered_map<int, unordered_set<int>> Q_light;
        unordered_map<int, unordered_set<int>> Q_heavy;

        // per x, the number of unique ys satisfying R
        unordered_map<int, int> degree;

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

        START_LOCAL("OSPG (Ea, Ec)");
        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex] = {vertex};
            degree[vertex] = 1;
        }
        
        END_LOCAL();
        // fast lookup on the first column of Eb
        unordered_map<int, unordered_set<int>> Eb; 
                
        START_LOCAL("OSPG (delta_R0, R0, Eb_reverse)");
        // The degree condition is trivially satisfied
        delta_R_prev = Ec;
        R_prev = Ec;

        // Build Eb_reverse jit
        if (!delta_R_prev.empty()) {
            // All other edges correspond to edges with label b
            for (const auto& [src, edges] : product.adjList) {
                // unordered_set<int> dst_set;
                for (const Edge& e : edges) {
                    // dst_set.insert(e.dest); 
                    Eb_reverse[e.dest].insert(src);
                }
            }
        }
        END_LOCAL();

        START_LOCAL("OSPG (R)");
        // Compute R(X, Y) satisfying degree(X) < bound
        unordered_map<int, unordered_set<int>> delta_R;
        while (!delta_R_prev.empty()) {
            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not R^{i-1}(X, Z)
            // 67ms (similar to below) for nn with 20000 vertices
            for (const auto& [y, zs] : delta_R_prev) {
                // lookup tuples in Eb
                auto xs = Eb_reverse[y];
                for (const auto& x: xs) {
                    int d = degree[x]; // create an entry with value 0 if not exists
                    auto z_it = zs.begin();
                    while ((d < bound) && (z_it!=zs.end())) {
                        int z = *z_it;
                        if (negate_prev(R_prev, x, z)) {
                            delta_R[x].insert(z);
                            ++d; 
                        }
                        ++z_it;
                    }
                    degree[x] = d;
                }
            }

            VERSIONED_IMPLEMENTATION("65ms for nn with 20000 vertices", {
                for (const auto& [y, zs] : delta_R_prev) {
                    // lookup tuples in Eb
                    auto xs = Eb_reverse[y];
                    for (const auto& x: xs) {
                        for (const auto& z: zs) {
                            if (negate_prev(R_prev, x, z)) {
                                if (degree[x] < bound) {
                                    delta_R[x].insert(z);
                                    degree[x] += 1;    
                                }
                            }
                        } 
                    }
                }
            });

            VERSIONED_IMPLEMENTATION("189248ms for nn with 20000 vertices", {
                for (const auto& [y, zs] : delta_R_prev) {
                    // lookup tuples in Eb
                    auto xs = Eb_reverse[y];
                    int zs_size = size(zs);
                    for (const auto& x: xs) {
                        if (R_prev.find(x) == R_prev.end()) {
                            // degree will be 0
                            if (zs_size <= bound) {
                                delta_R[x] = zs;
                                degree[x] = zs_size;
                            } else {
                                delta_R[x].insert(zs.begin(), std::next(zs.begin(), bound - 1)); // compared with delta_R[x]=z, the iterator-based merge is very expensive due to data movement, necessary for degree bound
                                degree[x] = bound;
                            }
                        } else if (degree[x] == bound) {
                            continue;
                        } else {
                            if (degree[x] + zs_size <= bound) {
                                delta_R[x] = zs;
                                degree[x] += zs_size;    
                            } else {
                                delta_R[x].insert(zs.begin(), std::next(zs.begin(), bound - degree[x] - 1));  
                                degree[x] = bound;
                            }
                        }
                    }
                }
            });
            
            // R^i(X, Y) = R^{i-1}(X, Y) or delta R^i(X, Y)
            for (const auto& [src, edges] : delta_R) {
                R_prev[src].insert(edges.begin(), edges.end());
            }
            delta_R_prev = std::move(delta_R);
            delta_R.clear();
        }
        R = std::move(R_prev);
        END_LOCAL();

        START_LOCAL("OSPG (Rl, Rh)");
        for (const auto& [x, y]: degree) {
            if (y == bound) {
                R_heavy.insert(x);
            } else {
                R_light[x] = R[x];
            }
        }
        END_LOCAL();

        START_LOCAL("OSPG (Ql)");
        // Compute Q_light (change the join order)
        // Ea is symmetric (Ql(X, Y) :- Rl(X, Y), Ea(X, X).)
        for (const auto& [x, ys] : R_light) {
            if (Ea.find(x)!=Ea.end()) {
                for (const auto& y: ys) {
                    Q_light[x].insert(y);
                }
            }
        }
        END_LOCAL();

        // Compute T using semi-naive
        unordered_map<int, unordered_set<int>> T;
        unordered_map<int, unordered_set<int>> T_prev;
        unordered_map<int, unordered_set<int>> delta_T_prev;

        START_LOCAL("OSPG (delta_T0, T0)");
        // delta T^0(Y, Y) :- R_heavy(Y), Ea(Y, Y) 
        for (const auto& y: R_heavy) {
            if (Ea.find(y) != Ea.end()) {
                delta_T_prev[y].insert(y);
            }
        }
        T_prev = delta_T_prev;
        END_LOCAL();

        START_LOCAL("OSPG (Eb)");
        // Build Eb only if delta_T_prev is greater than 0
        if (!delta_T_prev.empty()) {
            for (const auto& [src, edges] : product.adjList) {
                // unordered_set<int> dst_set;
                for (const Edge& e : edges) {
                    // dst_set.insert(e.dest); 
                    Eb[src].insert(e.dest);
                }
            }
        }
        END_LOCAL();

        START_LOCAL("OSPG (T)");
        while (!delta_T_prev.empty()) {
            unordered_map<int, unordered_set<int>> delta_T;
            // delta T^i(X, Y)  = delta T^{i-1}(X, Z) and Eb(Z, b, Y) and not T^{i-1}(X, Y)
            for (const auto& [x, zs] : delta_T_prev) {
                for (const auto& z: zs) {
                    auto ys = Eb[z];
                    for (const auto& y: ys) {
                        if (negate_prev(T_prev, x, y)) {
                            delta_T[x].insert(y);
                        }
                    }

                    VERSIONED_IMPLEMENTATION("Inlined with bulk movement", {
                        if (T_prev.find(x) == T_prev.end()) {
                            delta_T[x].insert(ys.begin(), ys.end());
                        } else {
                            for (const auto& y: ys) {
                                if (T_prev[x].find(y) == T_prev[x].end()) {
                                    delta_T[x].insert(y);
                                }
                            }
                        }
                    });
                }
            }
            
            // R^i(X, Y) = R^{i-1}(X, Y) or delta R^i(X, Y)
            for (const auto& [src, edges] : delta_T_prev) {
                T_prev[src].insert(edges.begin(), edges.end());
            }
            delta_T_prev = delta_T;
        }
        T = std::move(T_prev);
        END_LOCAL();

        START_LOCAL("OSPG (Qh)");
        for (const auto& [x, zs] : T) {
            for (const auto& z: zs) {
                for (const auto& y: Ec[z]) {
                    Q_heavy[x].insert(y);
                }
            }
        }
        END_LOCAL();

        START_LOCAL("OSPG (Ql + Qh)");
        Q_heavy.insert(Q_light.begin(), Q_light.end());
        END_LOCAL();
        return ReachablePairs(Q_heavy);
    }

    NFA query(NFA & data_nfa, const string& pattern) {
        // cout << "Data nfa" << endl;
        // data_nfa.print();
        NFA query_nfa = post2nfa(re2post(pattern)).getDFA();
        // cout << "Query NFA" << endl;
        // query_nfa.print();
        return query_nfa.product(data_nfa);
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
        
        while (!delta_prev.empty()) {
            unordered_map<int, unordered_set<int>> delta;
            for (const auto& [src, edges] : delta_prev) {
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
}
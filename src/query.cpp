#include "unordered_map"
#include "unordered_set"
#include <cmath>
#include "rpqdb/Graph.hpp"
#include "rpqdb/NFA.hpp"
#include "rpqdb/Profiler.hpp"
#include <iterator>
// #define DEBUG
#include <boost/container/flat_set.hpp>

// See https://www.boost.org/doc/libs/1_84_0/boost/container/flat_set.hpp
// Flatset is a sorted, unique associative container
// Similar to set, but implemented by an ordered sequence container instead (vector)

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
    ReachablePairs<boost::container::flat_set<int>> PG(Graph&& product) {
        unordered_set<int> Ea;  // reflexive
        unordered_map<int, boost::container::flat_set<int>> Ec; 
        unordered_map<int, boost::container::flat_set<int>> R;
        unordered_map<int, boost::container::flat_set<int>> T;

        unordered_map<int, boost::container::flat_set<int>> R_prev;
        unordered_map<int, boost::container::flat_set<int>> delta_R_prev;
        
        START_LOCAL("PG semi-naive (Ea, Ec)");
        // Add self-loops corresponding to edges with label a
        // create a copy, not move, since product is still used in later benchmark
        Ea = product.starting_vertices;
        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex] = {vertex};
        }
        END_LOCAL();

        #ifdef DEBUG
        cout << "PG seminaive Initial values" << endl;
        cout << "Ea" << endl;
        for (auto i: Ea) {
            cout << i << ", ";
        }
        cout << endl;
        cout << "Ec" << endl;
        for (auto [i, j]: Ec) {
            cout << i << ", ";
        }
        cout << endl;
        #endif

        // delta R_0 and R_0
        START_LOCAL("PG semi-naive (delta_R0, R0, Eb_reverse)");
        delta_R_prev = Ec;
        R_prev = Ec;

        unordered_map<int, boost::container::flat_set<int>> Eb_reverse; // fast lookup on the second column of Eb
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

        #ifdef DEBUG
        cout << endl << "Eb_reverse" << endl;

        for (auto [i, j]: Eb_reverse) {
            cout << i << ": ";
            for (auto k: j) {
                cout << k << ", ";
            }
            cout << endl;
        }
        #endif

        START_LOCAL("PG semi-naive (R)");
        while (!delta_R_prev.empty()) {
            unordered_map<int, boost::container::flat_set<int>> delta_R;

            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not R^{i-1}(X, Z)
            for (const auto& [y, zs] : delta_R_prev) {
                auto xs = Eb_reverse[y];
                for (const auto& x: xs) {
                    if (R_prev.find(x) == R_prev.end()) {
                        R_prev[x] = zs;
                        delta_R[x] = zs;
                    } else {
                        for (const auto& z: zs) {
                            if (R_prev[x].find(z) == R_prev[x].end()) {
                                delta_R[x].insert(z);
                                R_prev[x].insert(z);
                            }
                        }
                    }
                }
            }

            #ifdef DEBUG
            cout << endl << "Delta_R derived" << endl;    
            for (auto [i, j]: delta_R) {
                cout << i << ": ";
                for (auto k: j) {
                    cout << k << ", ";
                }
                cout << endl;
            }
            #endif

            delta_R_prev = std::move(delta_R);
        }
        R = std::move(R_prev);
        END_LOCAL();

        #ifdef DEBUG
        cout << endl << "R value" << endl;    
        for (auto [i, j]: R) {
            cout << i << ": ";
            for (auto k: j) {
                cout << k << ", ";
            }
            cout << endl;
        }
        #endif

        START_LOCAL("PG semi-naive (T)");
        // T(X, Z) = Ea(X, a, X), R(X, Z)
        for (const auto& [x, zs] : R) {
            if (Ea.find(x)!=Ea.end()){
                T[x] = zs;
            }
        }
        END_LOCAL();

        #ifdef DEBUG
        cout << "PG seminaive result" << endl;
        for (auto [i, j]: T) {
            cout << i << ": ";
            for (auto k: j) {
                cout << k << ", ";
            }
            cout << endl;
        }
        #endif
        return VectorReachablePairs(T);
    }

    ReachablePairs<std::unordered_set<int>> OSPG(Graph&& product) {
        // A bound for heavy/light partition of R
        // int bound = int(0.2*std::floor(std::sqrt(product.getEdges())))+1;

        int bound = std::floor(std::sqrt(product.getEdges()))+1;
        // int bound = 5;
        cout << "Degree bound is "<< bound << endl;

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
        unordered_map<int, int> degree; // ab*c-degree

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
        Ea = product.starting_vertices;

        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex] = {vertex};
            degree[vertex] = 1;
        }
        
        END_LOCAL();
                
        START_LOCAL("OSPG (delta_R0, R0, Eb_reverse)");
        // The degree condition is trivially satisfied
        delta_R_prev = Ec;
        R_prev = Ec;

        // Build Eb_reverse jit
        if (!delta_R_prev.empty()) {
            // All other edges correspond to edges with label b
            for (const auto& [src, edges] : product.adjList) {
                for (const Edge& e : edges) {
                    Eb_reverse[e.dest].insert(src);
                }
            }
        }
        END_LOCAL();

        START_LOCAL("OSPG (R)");
        // Compute R(X, Y) satisfying degree(X) < bound
        while (!delta_R_prev.empty()) {
            unordered_map<int, unordered_set<int>> delta_R;

            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not R^{i-1}(X, Z)
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
                            R_prev[x].insert(z);
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

            delta_R_prev = std::move(delta_R);
        }
        R = std::move(R_prev);
        END_LOCAL();

        START_LOCAL("OSPG (Rl, Rh)");
        for (const auto& [x, y]: degree) {
            if (y >= bound) {
                R_heavy.insert(x);
            } else {
                R_light[x] = R[x];
            }
        }
        END_LOCAL();

        #ifdef DEBUG
        cout << "R light" << endl;
        for (const auto& [source, destinations] : R_light) {
            std::cout << source << ": ";
            for (const auto& dest : destinations) {
                std::cout << dest << ", ";
            }
            std::cout << "\n";
        }
        cout << "R heavy" << endl;
        for (auto i : R_heavy){
            cout << i << " ";
        }
        cout << endl;
        #endif
        
        START_LOCAL("OSPG (Ql)");
        // Compute Q_light (change the join order)
        // Ea is symmetric (Ql(X, Y) :- Rl(X, Y), Ea(X, X).)
        
        for (const auto& [x, ys] : R_light) {
            if (Ea.count(x)) {
                Q_light[x] = std::move(ys);
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
        // fast lookup on the first column of Eb
        unordered_map<int, unordered_set<int>> Eb; 

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
                    if (Eb.find(z) != Eb.end()) {
                        auto ys = Eb[z];
                        if (T_prev.find(x) == T_prev.end()) {
                            delta_T[x] = ys;
                            T_prev[x] = ys;
                        } else {
                            for (const auto& y: ys) {
                                if (T_prev[x].find(y) == T_prev[x].end()) {
                                    delta_T[x].insert(y);
                                    T_prev[x].insert(y);
                                }
                            }
                        }                            
                    }
                }
            }

            delta_T_prev = std::move(delta_T);
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
        Q_light.insert(Q_heavy.begin(), Q_heavy.end());
        // Q_heavy.insert(Q_light.begin(), Q_light.end());
        END_LOCAL();

        #ifdef DEBUG
        cout << "OSPG results" << endl;
        for (const auto& [source, destinations] : Q_light) {
            std::cout << source << ": ";
            for (const auto& dest : destinations) {
                std::cout << dest << ", ";
            }
            std::cout << "\n";
        }
        #endif
        return Q_light;
    }

    NFA query(NFA & data_nfa, const string& pattern) {
        // cout << "Data nfa" << endl;
        // data_nfa.print();
        NFA query_nfa = post2nfa(re2post(pattern)).getDFA();
        // cout << "Query NFA" << endl;
        // query_nfa.print();
        return query_nfa.product(data_nfa);
    }

    ReachablePairs<std::set<int>> OSPG_OrderedSet(Graph&& product) {
        // Represent binary relations as insertion-ordered set
        // A bound for heavy/light partition of R
        int bound = std::floor(std::sqrt(product.getEdges()))+1;
        cout << "Degree bound is "<< bound << endl;
    
        // Ea simply looks up, still use unordered set (hash map)
        unordered_set<int> Ea;
        unordered_map<int, set<int>> Eb_reverse; // fast lookup on the second column of Eb
        unordered_map<int, set<int>> Ec;
    
        unordered_map<int, set<int>> R;
        unordered_map<int, set<int>> R_prev;
        unordered_map<int, set<int>> delta_R_prev;
    
        unordered_map<int, set<int>> R_light;
        set<int> R_heavy;
    
        unordered_map<int, set<int>> Q_light;
        unordered_map<int, set<int>> Q_heavy;
    
        // use the default size operator (constant time) instead of degree
        // unordered_map<int, int> degree; // ab*c-degree
    
        auto negate_prev = [](const unordered_map<int, set<int>>& prev, int x, int y) -> bool {
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
    
        START_LOCAL("OSPG_OrderedSet (Ea, Ec)");
        Ea = product.starting_vertices;
    
        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex] = {vertex};
        }
        END_LOCAL();
                
        START_LOCAL("OSPG_OrderedSet (delta_R0, R0, Eb_reverse)");
        // The degree condition is trivially satisfied
        delta_R_prev = Ec;
        R_prev = Ec;
    
        // Build Eb_reverse jit
        if (!delta_R_prev.empty()) {
            // All other edges correspond to edges with label b
            for (const auto& [src, edges] : product.adjList) {
                for (const Edge& e : edges) {
                    Eb_reverse[e.dest].insert(src);
                }
            }
        }
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedSet (R)");
        // Compute R(X, Y) satisfying degree(X) < bound
        while (!delta_R_prev.empty()) {
            unordered_map<int, set<int>> delta_R;
    
            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not R^{i-1}(X, Z)
            for (const auto& [y, zs] : delta_R_prev) {
                // lookup tuples in Eb
                auto xs = Eb_reverse[y];
                
                for (const auto& x: xs) {
                    int degree = R_prev[x].size();
                    auto z_it = zs.begin();
                    while ((degree < bound) && (z_it!=zs.end())) {
                        int z = *z_it;
                        if (negate_prev(R_prev, x, z)) {
                            delta_R[x].insert(z);
                            R_prev[x].insert(z);
                            degree++;
                        }
                        ++z_it;
                    }
                }
            }
    
            delta_R_prev = std::move(delta_R);
        }
        R = std::move(R_prev);
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedSet (Rl, Rh)");
        for (const auto& [x, ys]: R) {
            if (ys.size() >= bound) {
                R_heavy.insert(x);
            } else {
                R_light[x] = R[x];
            }
        }
        END_LOCAL();
    
        #ifdef DEBUG
        cout << "R light" << endl;
        for (const auto& [src, edges] : R_light) {
            cout << src << ": ";
            for (const auto& edge : edges) {
                cout << edge << ", ";
            }
            cout << endl;
        }
        cout << "R heavy" << endl;
        for (auto i : R_heavy){
            cout << i << " ";
        }
        cout << endl;
        #endif
        
        START_LOCAL("OSPG_OrderedSet (Ql)");
        // Compute Q_light (change the join order)
        // Ea is symmetric (Ql(X, Y) :- Rl(X, Y), Ea(X, X).)
        
        for (const auto& [x, ys] : R_light) {
            if (Ea.count(x)) {
                Q_light[x] = std::move(ys);
            }
        }
        END_LOCAL();
    
        // Compute T using semi-naive
        unordered_map<int, unordered_set<int>> T;
        unordered_map<int, unordered_set<int>> T_prev;
        unordered_map<int, unordered_set<int>> delta_T_prev;
    
        START_LOCAL("OSPG_OrderedSet (delta_T0, T0)");
        // delta T^0(Y, Y) :- R_heavy(Y), Ea(Y, Y) 
        for (const auto& y: R_heavy) {
            if (Ea.find(y) != Ea.end()) {
                delta_T_prev[y].insert(y);
            }
        }
        
        T_prev = delta_T_prev;
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedSet (Eb)");
        // fast lookup on the first column of Eb
        unordered_map<int, unordered_set<int>> Eb; 
    
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
    
        START_LOCAL("OSPG_OrderedSet (T)");
        while (!delta_T_prev.empty()) {
            unordered_map<int, unordered_set<int>> delta_T;
            // delta T^i(X, Y)  = delta T^{i-1}(X, Z) and Eb(Z, b, Y) and not T^{i-1}(X, Y)
            
            for (const auto& [x, zs] : delta_T_prev) {
                for (const auto& z: zs) {
                    if (Eb.find(z) != Eb.end()) {
                        auto ys = Eb[z];
                        if (T_prev.find(x) == T_prev.end()) {
                            delta_T[x] = ys;
                            T_prev[x] = ys;
                        } else {
                            for (const auto& y: ys) {
                                if (T_prev[x].find(y) == T_prev[x].end()) {
                                    delta_T[x].insert(y);
                                    T_prev[x].insert(y);
                                }
                            }
                        }                            
                    }
                }
            }
    
            delta_T_prev = std::move(delta_T);
        }
        T = std::move(T_prev);
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedSet (Qh)");
        for (const auto& [x, zs] : T) {
            for (const auto& z: zs) {
                for (const auto& y: Ec[z]) {
                    Q_heavy[x].insert(y);
                }
            }
        }
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedSet (Ql + Qh)");
        for (const auto& [x, ys]: Q_light) {
            for (const auto& y: ys){
                Q_heavy[x].insert(y);
            }
        }
        // Q_heavy.insert(Q_light.begin(), Q_light.end());
        END_LOCAL();
    
        #ifdef DEBUG
        cout << "OSPG_OrderedSet results" << endl;
        for (const auto& [src, edges] : Q_heavy) {
            cout << src << ": ";
            for (const auto& edge : edges) {
                cout << edge << ", ";
            }
            cout << endl;
        }
        #endif
        return ReachablePairs<std::set<int>>(Q_heavy);
    }
    
    ReachablePairs<boost::container::flat_set<int>> OSPG_OrderedVector(Graph&& product) {
        // A bound for heavy/light partition of R
        int bound = std::floor(std::sqrt(product.getEdges()))+1;
        cout << "Degree bound is "<< bound << endl;
    
        // Ea simply looks up, still use unordered set (hash map)
        unordered_set<int> Ea;
        unordered_map<int, vector<int>> Eb_reverse; // fast lookup on the second column of Eb
        unordered_map<int, boost::container::flat_set<int>> Ec;
    
        unordered_map<int, boost::container::flat_set<int>> R;
        unordered_map<int, boost::container::flat_set<int>> R_prev;
        unordered_map<int, boost::container::flat_set<int>> delta_R_prev;
    
        unordered_map<int, boost::container::flat_set<int>> R_light;
        unordered_set<int> R_heavy;
    
        unordered_map<int, boost::container::flat_set<int>> Q_light;
        unordered_map<int, boost::container::flat_set<int>> Q_heavy;
    
        // auto negate_prev = [](const unordered_map<int, vector<int>>& prev, int x, int y) -> bool {
        //     auto search = prev.find(x); 
        //     if (search == prev.end()) {
        //         return true; 
        //     } else {
        //         auto ans = search->second;
        //         if (ans.find(y) == ans.end()){
        //             return true;
        //         } else {
        //             return false;
        //         }
        //     }
        // };
    
        START_LOCAL("OSPG_OrderedVector (Ea, Ec)");
        Ea = product.starting_vertices;
    
        // Add self-loops corresponding to edges with label c
        for (const auto& vertex : product.accepting_vertices) {
            Ec[vertex].insert(vertex);
        }
        END_LOCAL();
                
        START_LOCAL("OSPG_OrderedVector (delta_R0, R0, Eb_reverse)");
        // The degree condition is trivially satisfied
        delta_R_prev = Ec;
        R_prev = Ec;
    
        // Build Eb_reverse jit
        if (!delta_R_prev.empty()) {
            // All other edges correspond to edges with label b
            for (const auto& [src, edges] : product.adjList) {
                for (const Edge& e : edges) {
                    Eb_reverse[e.dest].push_back(src);
                }
            }
        }
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedVector (R)");
        while (!delta_R_prev.empty()) {
            unordered_map<int, boost::container::flat_set<int>> delta_R;
    
            // delta R^i(X, Z)  = delta R^{i-1}(Y, Z) and Eb(X, b, Y) and not delta R prev
            for (const auto& [y, zs] : delta_R_prev) {
                // lookup tuples in Eb
                auto xs = Eb_reverse[y];
                
                for (const auto& x: xs) {
                    if (R_prev.find(x) == R_prev.end()){
                        auto rhs = boost::container::flat_set<int>(zs.begin(), min(zs.begin()+bound-1, zs.end()));
                        R_prev[x] = rhs;
                        delta_R[x] = rhs;
                    } else {
                        for (const auto& z: zs) {
                            if (R_prev[x].find(z) == R_prev[x].end() && R_prev[x].size() < bound) {
                                delta_R[x].insert(z);
                                R_prev[x].insert(z);
                            }
                        }
                    }
                }
            }
            delta_R_prev = std::move(delta_R);
        }
        R = std::move(R_prev);
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedVector (Rl, Rh)");
        for (const auto& [x, ys]: R) {
            if (ys.size() >= bound) {
                R_heavy.insert(x);
            } else {
                R_light[x] = R[x];
            }
        }
        END_LOCAL();
    
        #ifdef DEBUG
        cout << "R light" << endl;
        for (const auto& [src, edges] : R_light) {
            cout << src << ": ";
            for (const auto& edge : edges) {
                cout << edge << ", ";
            }
            cout << endl;
        }
        cout << "R heavy" << endl;
        for (auto i : R_heavy){
            cout << i << " ";
        }
        cout << endl;
        #endif
        
        START_LOCAL("OSPG_OrderedVector (Ql)");
        // Compute Q_light (change the join order)
        // Ea is symmetric (Ql(X, Y) :- Rl(X, Y), Ea(X, X).)
        
        for (const auto& [x, ys] : R_light) {
            if (Ea.count(x)) {
                Q_light[x] = std::move(ys);
            }
        }
        END_LOCAL();
        
        // Compute T using semi-naive
        unordered_map<int, boost::container::flat_set<int>> T;
        unordered_map<int, boost::container::flat_set<int>> T_prev;
        unordered_map<int, boost::container::flat_set<int>> delta_T_prev;
    
        START_LOCAL("OSPG_OrderedVector (delta_T0, T0)");
        // delta T^0(Y, Y) :- R_heavy(Y), Ea(Y, Y) 
        for (const auto& y: R_heavy) {
            if (Ea.find(y) != Ea.end()) {
                delta_T_prev[y].insert(y);
            }
        }
        
        T_prev = delta_T_prev;
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedVector (Eb)");
        // fast lookup on the first column of Eb
        unordered_map<int, boost::container::flat_set<int>> Eb; 
    
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
    
        START_LOCAL("OSPG_OrderedVector (T)");
        while (!delta_T_prev.empty()) {
            unordered_map<int, boost::container::flat_set<int>> delta_T;
            // delta T^i(X, Y)  = delta T^{i-1}(X, Z) and Eb(Z, b, Y) and not T^{i-1}(X, Y)
            
            for (const auto& [x, zs] : delta_T_prev) {
                for (const auto& z: zs) {
                    if (Eb.find(z) != Eb.end()) {
                        auto ys = Eb[z];
                        if (T_prev.find(x) == T_prev.end()) {
                            delta_T[x] = ys;
                            T_prev[x] = ys;
                        } else {
                            for (const auto& y: ys) {
                                if (T_prev[x].find(y) == T_prev[x].end()) {
                                    delta_T[x].insert(y);
                                    T_prev[x].insert(y);
                                }
                            }
                        }                            
                    }
                }
            }
    
            delta_T_prev = std::move(delta_T);
        }
        T = std::move(T_prev);
        END_LOCAL();
    
        START_LOCAL("OSPG_OrderedVector (Qh)");
        for (const auto& [x, zs] : T) {
            for (const auto& z: zs) {
                for (const auto& y: Ec[z]) {
                    Q_heavy[x].insert(y);
                }
            }
        }
        END_LOCAL();
    
        if (Q_heavy.size() < Q_light.size()) {
            START_LOCAL("OSPG_OrderedVector (Ql + Qh)");    
            for (const auto& [x, ys]: Q_heavy) {
                for (const auto& y: ys){
                    Q_light[x].insert(y);
                }
            }    
            END_LOCAL();
            #ifdef DEBUG
            cout << "OSPG_OrderedVector results" << endl;
            for (const auto& [src, edges] : Q_light) {
                cout << src << ": ";
                for (const auto& edge : edges) {
                    cout << edge << ", ";
                }
                cout << endl;
            }
            #endif
            return ReachablePairs<boost::container::flat_set<int>>(Q_light);
        } else {
            START_LOCAL("OSPG_OrderedVector (Ql + Qh)");    
            for (const auto& [x, ys]: Q_light) {
                for (const auto& y: ys){
                    Q_heavy[x].insert(y);
                }
            }
            END_LOCAL();
            #ifdef DEBUG
            cout << "OSPG_OrderedVector results" << endl;
            for (const auto& [src, edges] : Q_heavy) {
                cout << src << ": ";
                for (const auto& edge : edges) {
                    cout << edge << ", ";
                }
                cout << endl;
            }
            #endif
            return ReachablePairs<boost::container::flat_set<int>>(Q_heavy);
        }
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
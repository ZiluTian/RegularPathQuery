#ifndef RPQDB_Graph_H
#define RPQDB_Graph_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <queue>
#include <tuple>
#include <functional>
#include <stack>
#include <algorithm>

#include "NFA.hpp"
#include "rpqdb/Profiler.hpp"

// Custom hash for StatePair
using StatePair = std::pair<rpqdb::State*, int>;

namespace std {
    template<>
    struct hash<StatePair> {
        size_t operator()(const StatePair& sp) const {
            // Combine hash of the pointer and the integer
            size_t h1 = hash<rpqdb::State*>()(sp.first);   // Hash the pointer
            size_t h2 = hash<int>()(sp.second);     // Hash the int
            return h1 ^ (h2 << 1);  // Combine hashes (avoid XOR symmetry)
        }
    };
}

namespace rpqdb{
    using namespace std;

    // Represents a labeled edge in the graph
    struct Edge {
        string label;
        int dest;
    };
    
    // Allow fine tuning the data structure for the result
    class ReachablePairs {
        private:
            unordered_map<int, unordered_set<int>>result;

        public:
            // Constructor that takes an existing map
            ReachablePairs(const std::unordered_map<int, std::unordered_set<int>>& initialResult)
            : result(initialResult) {}  // Member initializer list copies the input

            // Default constructor (optional)
            ReachablePairs() = default;  // Creates empty map

            void addPair(int x, int y) {
                result[x].insert(y);
            }

            void print() {
                cout << "Reachable pairs" << endl;
                for (const auto& [src, edges] : result) {
                    cout << src << ": ";
                    for (const auto& edge : edges) {
                        cout << edge << ", ";
                    }
                    cout << endl;
                }
            }
    };

    // Graph class stores adjacency list representation
    class Graph {   
    private:
        int totalEdges = 0;
        
        void addEdge(int v1, const string& label, int v2) {
            adjList[v1].push_back({label, v2});
            vertices.insert(v1);
            vertices.insert(v2);
            totalEdges += 1;
        }

    public:
        unordered_map<int, vector<Edge>> adjList;
        unordered_set<int> vertices;
        // directed graph, can leave empty
        unordered_set<int> starting_vertices;
        unordered_set<int> accepting_vertices;
        
        int getEdges() {
            return totalEdges;
        }
    
        // A debug method for visualizing the content of the graph
        void print(){
            cout << "Starting vertices: ";
            for (const auto& n: starting_vertices) {
                cout << n << ", ";
            }
            cout << endl;

            cout << "Ending vertices: ";
            for (const auto& n: accepting_vertices) {
                cout << n << ", ";
            }
            cout << endl;
        
            for (const auto& [src, edges] : adjList) {
                cout << src << ": ";
                for (const auto& edge : edges) {
                    cout << "(" << edge.label << " -> " << edge.dest << ") ";
                }
                cout << endl;
            }
        }

        // Serialization format: v1 label v2
        void buildFromFile(const string& filename, const string& separator) {
            ifstream file(filename);
            if (!file.is_open()) {
                cerr << "Error: Unable to open file " << filename << " in the current directory." << endl;
                return;
            }
            string line;
            
            while (getline(file, line)) {
                size_t pos1 = line.find(separator);
                size_t pos2 = line.rfind(separator);
                if (pos1 != string::npos && pos2 != string::npos && pos1 != pos2) {
                    int v1 = stoi(line.substr(0, pos1));
                    string label = line.substr(pos1+1, pos2-pos1-1);
                    int v2 = stoi(line.substr(pos2+1));
                    addEdge(v1, label, v2);
                }
            }
        }

        // Construct a product graph from a DFA
        Graph product(NFA& dfa) {
            Graph result;
            int product_vertex_id = 0;
			unordered_map<StatePair, int> state_map;
            State * start1 = dfa.start_state;
            
            // Helper function to get or create a new state in the product NFA
			auto get_or_create_vertex = [&](State* s1, int s2) -> int {
				StatePair key = {s1, s2};
				if (state_map.find(key) == state_map.end()) {
					product_vertex_id += 1;
					state_map[key] = product_vertex_id;
				}
				return state_map[key];
			};

            // Perform a breadth-first search (BFS) to explore all reachable state pairs
			queue<StatePair> queue;
            unordered_set<StatePair> visited;

            for (const auto& x : vertices) {
                queue.push({start1, x});
                // Get the corresponding state in the product NFA
				int product_state = get_or_create_vertex(start1, x);
                result.starting_vertices.insert(product_state);
            }

			while (!queue.empty()) {
				auto [current1, current2] = queue.front();
				queue.pop();
                visited.insert({current1, current2});

				// Get the corresponding state in the product NFA
				int current_product_state = get_or_create_vertex(current1, current2);
                if (current1->is_accepting) {
                    result.accepting_vertices.insert(current_product_state);
                }

				// Process transitions
				for (const auto& trans1 : current1->transitions) {
					for (const auto& trans2 : adjList[current2]) {
						State* next1 = trans1.target;
						if (trans1.label == trans2.label) {
							int next_product_state = get_or_create_vertex(next1, trans2.dest);
							result.addEdge(current_product_state, trans1.label, next_product_state);
							// check if the node has been visited
                            if (visited.find({next1, trans2.dest}) == visited.end()){
                                queue.push({next1, trans2.dest});
                            }
						}
					}
				}
			}

			return result;
        }

        ReachablePairs PG() {
            START_LOCAL("BFS");

            ReachablePairs result;
            if (size(starting_vertices) == 0 || size(accepting_vertices) == 0) {
                return result;
            }
            if (adjList.empty()){
                return result;
            }

            unordered_set<int> visited;
            // std::vector<bool> visited(adjList.size(), false);

            // For each starting vertex, perform BFS to find reachable accepting vertices
            for (int start : starting_vertices) {
                if (visited.count(start)){
                // if (visited[start]){
                    continue;
                }

                std::queue<int> q;
                
                q.push(start);
                visited.insert(start);
                // visited[start] = true;

                while (!q.empty()) {
                    int current = q.front();
                    q.pop();
                    
                    // If current is an accepting state, record the (start, accept) pair
                    if (accepting_vertices.count(current)) {
                        result.addPair(start, current);
                    }
                    
                    // Explore all neighbors
                    for (const auto& edge : adjList[current]) {
                        int neighbor = edge.dest;
                        if (neighbor < visited.size() && !visited.count(neighbor)) {
                        // if (neighbor < visited.size() && !visited[neighbor]) {
                            // visited[neighbor] = true;
                            visited.insert(neighbor);
                            q.push(neighbor);
                        }
                    }
                }
            }

            END_LOCAL();
            return result;
        }

        ReachablePairs PGNaive() {
            START_LOCAL("BFS-Naive");
            ReachablePairs result;
            unordered_set<int> visited;

            // For each starting vertex, perform BFS to find reachable accepting vertices
            for (int start : starting_vertices) {
                if (visited.count(start)){
                    continue;
                }

                std::queue<int> q;
                
                q.push(start);
                visited.insert(start);

                while (!q.empty()) {
                    int current = q.front();
                    q.pop();
                    
                    // If current is an accepting state, record the (start, accept) pair
                    if (accepting_vertices.count(current)) {
                        result.addPair(start, current);
                    }
                    
                    // Explore all neighbors
                    for (const auto& edge : adjList[current]) {
                        int neighbor = edge.dest;
                        if (neighbor < visited.size() && !visited.count(neighbor)) {
                            visited.insert(neighbor);
                            q.push(neighbor);
                        }
                    }
                }
            }

            END_LOCAL();
            return result;
        }

        NFA constructDFA(int start_vertex, set<int> accepting_vertices) {
            NFA nfa;
            std::map<int, State*> vertex_to_state;
    
            // Create NFA states for each vertex
            for (int vertex : vertices) {
                vertex_to_state[vertex] = nfa.create_state(vertex);
            }
    
            // Add transitions to the NFA
            for (const auto& [src, edges] : adjList) {
                State* from_state = vertex_to_state[src];
                for (const Edge& edge : edges) {
                    State* to_state = vertex_to_state[edge.dest];
                    nfa.add_transition(from_state, to_state, edge.label);
                }
            }
    
            // Set the start state
            nfa.start_state = vertex_to_state[start_vertex];
    
            // Set accepting states
            for (int vertex : accepting_vertices) {
                vertex_to_state[vertex]->is_accepting = true;
            }
            
            return nfa.getDFA();
        }
    
        const unordered_map<int, vector<Edge>>& getAdjacencyList() const {
            return adjList;
        }
    
        const unordered_set<int>& getVertices() const {
            return vertices;
        }
    };
} // namespace rpqdb

#endif
#ifndef RPQDB_Graph_H
#define RPQDB_Graph_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <set>
#include <queue>
#include <tuple>
#include <functional>
#include <stack>
#include <algorithm>

#include "NFA.hpp"

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
            
    // Graph class stores adjacency list representation
    class Graph {   
    public:
        unordered_map<int, vector<Edge>> adjList;
        unordered_set<int> vertices;

        void addEdge(int v1, const string& label, int v2) {
            adjList[v1].push_back({label, v2});
            vertices.insert(v1);
            vertices.insert(v2);
        }
    
        // A debug method for visualizing the content of the graph
        void print(){
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
            }

			while (!queue.empty()) {
				auto [current1, current2] = queue.front();
				queue.pop();
                visited.insert({current1, current2});

				// Get the corresponding state in the product NFA
				int current_product_state = get_or_create_vertex(current1, current2);
	
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
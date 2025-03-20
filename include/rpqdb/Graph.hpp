#ifndef RPQDB_Graph_H
#define RPQDB_Graph_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <tuple>
#include <functional>
#include <stack>
#include <algorithm>

#include "NFA.hpp"

namespace rpqdb{
    using namespace std;

    // Represents a labeled edge in the graph
    struct Edge {
        string label;
        int dest;
    };
            
    // Graph class stores adjacency list representation
    class Graph {
    private:
        unordered_map<int, vector<Edge>> adjList;
        unordered_set<int> vertices;
    
    public:
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
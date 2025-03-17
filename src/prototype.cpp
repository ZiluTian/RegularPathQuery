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
#include "test.hpp"

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

    // Serialization format: v1 label v2
    void loadFromFile(const string& filename, const string& separator) {
        ifstream file(filename);
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

        return nfa.toDFA();
    }

    const unordered_map<int, vector<Edge>>& getAdjacencyList() const {
        return adjList;
    }

    const unordered_set<int>& getVertices() const {
        return vertices;
    }
};

NFA query(NFA && data_nfa, const string& pattern) {
    // cout << "Data nfa" << endl;
    // data_nfa.print();
    NFA query_nfa = post2nfa(re2post(pattern)).toDFA();
    // cout << "Query NFA" << endl;
    // query_nfa.print();
    return query_nfa.product(std::move(data_nfa));
}


int main() {
    Graph graph;
    graph.loadFromFile("graph.txt", " ");
    NFA data_nfa = graph.constructDFA(1, {11});

    ASSERT_TRUE(data_nfa.accepts("helloworld"));
    ASSERT_FALSE(data_nfa.accepts("hello"));
    ASSERT_FALSE(data_nfa.accepts("world"));
    ASSERT_FALSE(data_nfa.accepts("hel*oworld"));
    ASSERT_FALSE(data_nfa.accepts("hel*o*world"));

    ASSERT_TRUE(query(std::move(data_nfa), "hel*oworld").accepts("helloworld"));
    ASSERT_TRUE(query(std::move(data_nfa), "hel*o*wo*rld").accepts("helloworld"));
    
    return 0;
}
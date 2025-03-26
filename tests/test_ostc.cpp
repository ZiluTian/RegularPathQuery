#include <vector>
#include <memory>
#include <stdexcept>

#include "rpqdb/Graph.hpp"
#include "tests.hpp"
#include "query.cpp"

using namespace rpqdb;

int main(int argc, char **argv) {
    Graph graph;
    // path relative to the binary (here in the local build)
    string mySrcDir = MY_SRC_DIR;

    graph.buildFromFile(mySrcDir + "/resources/graph_tc.txt", " ");
    unordered_map<int, unordered_set<int>> ans = ostc(graph);

    for (const auto& [src, edges] : ans) {
        // lookup for e in edges in E
        std::cout << "Edge " << src << " : ";
        for (const auto& e: edges) {
            std::cout << e << ", ";
        }
        std::cout << std::endl;
    }

    return 0;
}
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
    // unordered_map<int, unordered_set<int>> ans = ostc(graph);

    graph.starting_vertices = {1, 2};
    graph.accepting_vertices = {1, 2, 3, 4, 5};

    graph.PG().print();
    cout << endl;
    PG(graph).print();

    return 0;
}
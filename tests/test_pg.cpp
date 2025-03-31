#include <vector>
#include <memory>
#include <stdexcept>

#include "rpqdb/Graph.hpp"
#include "tests.hpp"
#include "query.cpp"

using namespace rpqdb;


void example61() {
    std::vector<int> sizes = {10, 100, 1000, 10000, 100000};
    string query = "b*c";
    NFA query_dfa = post2nfa(re2post(query)).getDFA();    

    vector<tuple<int, long long, long long, long long>> results; 
    for (int size : sizes) {
        Graph graph;
        // path relative to the binary (here in the local build)
        string mySrcDir = MY_SRC_DIR;
        graph.buildFromFile(mySrcDir + "/resources/path_" + std::to_string(size) + ".txt", " ");
        Graph product = graph.product(query_dfa);

        long long pg_time = benchmark([&]() {
            PG(product);
        });
        long long ospg_time = benchmark([&]() {
            OSPG(product);
        });
        long long bfs_pg_time = benchmark([&]() {
            product.PG();
        });
        results.emplace_back(size, bfs_pg_time, pg_time, ospg_time);
    }
    
    cout << "Benchmark Results (ms):" << endl;
    cout << "-------------------------------------------------" << endl;
    cout << "Graph Size\tBFS-PG \t\tPG\t\tOSPG" << endl;
    cout << "-------------------------------------------------" << endl;
    for (const auto& [size, pTime, pgTime, ospgTime] : results) {
        cout << size << "\t\t" << pTime << "\t\t" << pgTime << "\t\t" << ospgTime << endl;
    }
    return;
}

void example62() {
    std::vector<int> sizes = {10, 100, 1000, 10000, 100000};
    string query = "ab*c";
    NFA query_dfa = post2nfa(re2post(query)).getDFA();    

    vector<tuple<int, long long, long long, long long>> results;  
    for (int size : sizes) {
        // cout << "Run example 6.2 with size " << size << endl;
        Graph graph;
        // path relative to the binary (here in the local build)
        string mySrcDir = MY_SRC_DIR;
        graph.buildFromFile(mySrcDir + "/resources/disjoint_cycles_" + std::to_string(size) + ".txt", " ");
        Graph product = graph.product(query_dfa);

        long long pg_time = benchmark([&]() {
            PG(product);
        });
        long long ospg_time = benchmark([&]() {
            OSPG(product);
        });
        long long bfs_pg_time = benchmark([&]() {
            product.PG();
        });
        results.emplace_back(size, bfs_pg_time, pg_time, ospg_time);
    }
    
    cout << "Benchmark Results (ms):" << endl;
    cout << "-------------------------------------------------" << endl;
    cout << "Graph Size\tBFS-PG\t\tPG\t\tOSPG" << endl;
    cout << "-------------------------------------------------" << endl;
    for (const auto& [size, pTime, pgTime, ospgTime] : results) {
        cout << size << "\t\t" << pTime << "\t\t" << pgTime << "\t\t" << ospgTime << endl;
    }
    return;
}

int main(int argc, char **argv) {
    example61();
    example62();
    return 0;
}
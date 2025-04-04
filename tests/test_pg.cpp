#include <vector>
#include <memory>
#include <stdexcept>

#include "rpqdb/Graph.hpp"
#include "rpqdb/Profiler.hpp"
#include "tests.hpp"
#include "query.cpp"

using namespace rpqdb;

class QueryGraphClass {
    public:
    string query;
    string graph_name_prefix;

    // Constructor
    QueryGraphClass(const std::string& q, const std::string& prefix)
        : query(q), graph_name_prefix(prefix) {
        std::cout << "QueryGraphClass constructed.\n";
    }

    // Destructor
    ~QueryGraphClass() {
        std::cout << "QueryGraphClass destroyed.\n";
    }

    void run(int size, const string& profile_name = "profile.dat") {
        EventProfiler::reset();
    
        NFA query_dfa = post2nfa(re2post(query)).getDFA();    
        Graph graph;

        START_LOCAL("Load graph size " + to_string(size));
        string mySrcDir = MY_SRC_DIR;
        graph.buildFromFile(mySrcDir + "/resources/" + graph_name_prefix + std::to_string(size) + ".txt", " ");
        END_LOCAL();

        START_LOCAL("Build product graph");
        Graph&& product = graph.product(query_dfa);
        END_LOCAL();

        // START_LOCAL("BFS total");
        product.PG();
        // END_LOCAL();
    
        // START_LOCAL("BFS (naive) total");
        product.PGNaive();
        // END_LOCAL();

        // START_LOCAL("Semi-naive PG total");
        PG(std::move(product));
        // END_LOCAL();

        // START_LOCAL("OSPG total");
        OSPG(std::move(product));
        // END_LOCAL();
    
        EventProfiler::export_to_file(profile_name);
        return;
    }
};

int main(int argc, char **argv) {
    int size = 1000000;

    QueryGraphClass ex = QueryGraphClass("b*c", "pathbsc_");
    QueryGraphClass ex61 = QueryGraphClass("b*c", "path_");
    QueryGraphClass ex62 = QueryGraphClass("ab*c", "disjoint_cycles_");

    ex61.run(size, "ex61profile_" + to_string(size) + ".dat");
    ex62.run(size, "ex62profile_"+ to_string(size) + ".dat");
    ex.run(size, "exbscprofile_"+ to_string(size) + ".dat");
    return 0;
}
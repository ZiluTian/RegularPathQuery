#include <vector>
#include <memory>
#include <stdexcept>

#include "rpqdb/Graph.hpp"
#include "rpqdb/Profiler.hpp"
#include "tests.hpp"
#include "query.cpp"

using namespace rpqdb;

class PGTest : public QueryGraphClass {
    public:
    using QueryGraphClass :: QueryGraphClass;
    
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
    int size = 20000;

    if (argc > 1) {
        size = atoi(argv[1]);
        if (size <= 0) {
            std::cerr << "Error: Size must be a positive integer\n";
            return 1;
        }
    } else {
        std::cout << "Using default size: " << size << "\n";
        std::cout << "To specify a different size, run: " << argv[0] << " <size>\n";
    }

    PGTest expath = PGTest("b*c", "pathbsc_");
    PGTest ex61 = PGTest("b*c", "path_");
    PGTest ex62 = PGTest("ab*c", "disjoint_cycles_");
    PGTest exnn = PGTest("ab*c", "nn_binary_classifier_");

    ex61.run(size, "ex61profile_" + to_string(size) + ".dat");
    ex62.run(size, "ex62profile_"+ to_string(size) + ".dat");
    expath.run(size, "exbscprofile_"+ to_string(size) + ".dat");
    exnn.run(size, "exnn_binary_profile_" + to_string(size) + ".dat");

    return 0;
}
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
        Graph product;

        // Avoid building product graph
        START_LOCAL("Load graph size " + to_string(size));
        string mySrcDir = MY_SRC_DIR;
        product.buildLabelledGraphFromFile(mySrcDir + "/resources/" + graph_name_prefix + std::to_string(size) + ".txt", " ");
        END_LOCAL();

        PG(std::move(product));
        OSPG(std::move(product));    
        
        EventProfiler::export_to_file(profile_name);
        return;
    }
};

int main(int argc, char **argv) {
    int size = 2000;
    
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

    QueryGraphClass ex = QueryGraphClass("ab*c", "nn_heavy_hitters_");
    std::string output_filename = "exnn_binary_profile_" + std::to_string(size) + ".dat";
    ex.run(size, output_filename);
    return 0;
}
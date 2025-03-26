#include "rpqdb/Graph.hpp"
#include <vector>
#include "tests.hpp"
#include "query.cpp"
#include "rpqdb/NFA.hpp"

using namespace rpqdb;

class GraphFixtureTestSuite {
    private:
        const Graph graph;
        std::vector<bool (*)(Graph &)> tests;

    public:
        explicit GraphFixtureTestSuite(Graph g)
            : graph(g){}
};

void fixture_test(Graph & g, bool (*func)(Graph &)){
    if (func(g)){
        cout << "All tests passed!" << endl;
    } else {
        cout << "One or more tests failed!" << endl;
    }
}

bool test_graphDFA1(Graph & graph){
    cout << "Started test graph DFA 1" << endl;
    auto data_nfa = graph.constructDFA(1, {11});
    ASSERT_TRUE(data_nfa.accepts("helloworld"));
    ASSERT_FALSE(data_nfa.accepts("hello"));
    ASSERT_FALSE(data_nfa.accepts("world"));
    ASSERT_FALSE(data_nfa.accepts("hel*oworld"));
    ASSERT_FALSE(data_nfa.accepts("hel*o*world"));
    ASSERT_TRUE(query(data_nfa, "hel*oworld").accepts("helloworld"));
    ASSERT_TRUE(query(data_nfa, "hel*o*wo*rld").accepts("helloworld"));
    return true;
}

bool test_graph2DFA1(Graph & graph){
    cout << "Started test graph 2 DFA 1" << endl;
    auto data_nfa = graph.constructDFA(1, {11});
    ASSERT_TRUE(data_nfa.accepts("helloworld"));
    ASSERT_TRUE(data_nfa.accepts("heloworld"));
    ASSERT_FALSE(data_nfa.accepts("hello"));
    ASSERT_FALSE(data_nfa.accepts("world"));
    ASSERT_FALSE(data_nfa.accepts("hel*oworld"));
    ASSERT_FALSE(data_nfa.accepts("hel*o*world"));
    ASSERT_TRUE(query(data_nfa, "hel*oworld").accepts("helloworld"));
    ASSERT_TRUE(query(data_nfa, "hel*oworld").accepts("heloworld"));
    return true;
}

bool test_graphDFA2(Graph & graph){
    cout << "Started test graph DFA 2" << endl;
    auto data_nfa = graph.constructDFA(4, {7});
    ASSERT_TRUE(data_nfa.accepts("low"));
    ASSERT_FALSE(data_nfa.accepts("hello"));
    ASSERT_TRUE(query(data_nfa, "l*o*w").accepts("low"));
    return true;
}

bool test_productGraph(Graph & graph){
    NFA nfa1 = post2nfa(re2post("ab*"));
    NFA dfa1 = nfa1.getDFA();
    cout << " print dfa " << endl;
    dfa1.print();
    cout << " print graph " << endl;
    graph.print();
    cout << " print product graph " << endl;
    Graph ans = graph.product(dfa1);
    ans.print();
    return true;
}

int main() {
    Graph graph;
    // path relative to the binary (here in the local build)
    string mySrcDir = MY_SRC_DIR;
    cout << "Load graph from " << mySrcDir << "/resources" << endl;
    graph.buildFromFile(mySrcDir + "/resources/graph1.txt", " ");
    cout << "Successfully loaded graph 1!" << endl;
    // graph.print();
    fixture_test(graph, test_graphDFA1);
    fixture_test(graph, test_graphDFA2);

    Graph graph2;
    graph2.buildFromFile(mySrcDir + "/resources/graph2.txt", " ");
    cout << "Successfully loaded graph 2!" << endl;
    // graph.print();
    fixture_test(graph2, test_graph2DFA1);
    fixture_test(graph2, test_graphDFA2);

    Graph graph3;
    graph3.buildFromFile(mySrcDir + "/resources/graph_tc.txt", " ");
    cout << "Successfully loaded graph_tc!" << endl;
    fixture_test(graph3, test_productGraph);
    return 0;
}
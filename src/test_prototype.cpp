#include "prototype.hpp"
#include <iostream>
#include <cassert> // For ASSERT_TRUE and ASSERT_FALSE

void fixture_test(Graph & g, bool (*func)(Graph &)){
    if (func(g)){
        cout << "All tests passed!" << endl;
    } else {
        cout << "One or more tests failed!" << endl;
    }
}

bool test_graphDFA1(Graph & graph){
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

bool test_graphDFA2(Graph & graph){
    auto data_nfa = graph.constructDFA(4, {7});
    ASSERT_TRUE(data_nfa.accepts("low"));
    ASSERT_FALSE(data_nfa.accepts("hello"));
    ASSERT_TRUE(query(data_nfa, "l*o*w").accepts("low"));
    return true;
}

int main() {
    Graph graph;
    graph.loadFromFile("resources/graph1.txt", " ");

    fixture_test(graph, test_graphDFA1);
    fixture_test(graph, test_graphDFA2);
    return 0;
}
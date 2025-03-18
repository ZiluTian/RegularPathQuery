#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

#include "NFA.hpp"
#include "test.hpp"

using namespace std;

bool recognizes(const string& l, const string& word) {
	NFA nfa1 = post2nfa(re2post(l));
	return nfa1.accepts(word);
}

bool testAccept() {
	ASSERT_TRUE(recognizes("a", "a"));
	ASSERT_TRUE(recognizes("b*", ""));
	ASSERT_FALSE(recognizes("b*", "a"));
	ASSERT_FALSE(recognizes("ab*c", "c"));
	ASSERT_TRUE(recognizes("ab*c", "ac"));
	ASSERT_FALSE(recognizes("ab*c", "a"));
	ASSERT_FALSE(recognizes("ab*c", "c"));
	ASSERT_FALSE(recognizes("ab*c", "b"));
	ASSERT_TRUE(recognizes("ab*c", "abc"));
	ASSERT_FALSE(recognizes("ab*c", "abbbb"));
	ASSERT_TRUE(recognizes("ab*c", "abbbbbc"));
	ASSERT_FALSE(recognizes("a(b|c)*d", "abbbb"));
	ASSERT_TRUE(recognizes("a(b|c)*d", "abbcbcbd"));
	ASSERT_TRUE(recognizes("a(b|c)*d", "abbbbbd"));
	return true;
}

void toDFATest() {
	NFA nfa1 = post2nfa(re2post("ab*c"));
	NFA nfa2 = post2nfa(re2post("ac"));

	NFA dfa1 = nfa1.getDFA();
	NFA dfa2 = nfa2.getDFA();
	NFA ans = dfa1.product(dfa2);
	// cout << "DFA 1" << endl;
	// dfa1.print();
	// cout << "DFA 2" << endl;
	// dfa2.print();
	// cout << "Product DFA" << endl;
	// ans.print();
}


int main(int argc, char **argv) {
	RUN_TEST(testAccept);
	toDFATest();
	return 0;
}
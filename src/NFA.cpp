#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

#include "NFA.hpp"

using namespace std;

int main(int argc, char **argv) {
	int i;
	string post;

	// if(argc < 3){
	// 	fprintf(stderr, "usage: nfa regexp string...\n");
	// 	return 1;
	// }
	
	post = re2post(argv[1]);
	cout << post << endl;	
	NFA start = post2nfa(post);
	start.print();
	return 0;
}


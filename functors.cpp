#include <cstdint>
#include "souffle/SouffleInterface.h"
#include <unordered_map>              // For storing counters
#include <atomic>                     // For atomic counters
#include <mutex>                      // For thread safety

// Global map to store atomic counters for each unique input value
std::unordered_map<int64_t, std::atomic<int64_t>> counters;
std::mutex map_mutex; // Mutex to protect the map

extern "C" {
    
    int64_t degree(int64_t x) {
        // Return the current value of the counter and increment it
        return counters[x];
    }
    
    int64_t inc_degree(int64_t x) {
        std::lock_guard<std::mutex> lock(map_mutex); // Ensure thread safety

        // Initialize the counter for `x` if it doesn't exist
        if (counters.find(x) == counters.end()) {
            counters[x] = 0; // Initialize to 0
        }
        counters[x]++;
        // Return the current value of the counter and increment it
        return x;
    }
    
// souffle::RamDomain bounded_degree(souffle::SymbolTable* symbolTable, souffle::RecordTable* recordTable, souffle::RamDomain arg) {{
//     assert(symbolTable && "NULL symbol table");
//     assert(recordTable && "NULL record table");

//     int count = 0;

//     // does unpack return the currently stored arg? 
//     const souffle::RamDomain* myTuple0 = recordTable->unpack(arg, 1);
//     souffle::RamDomain myTuple1[2] = {myTuple0[0], myTuple0[1] + 1};
//         // Return [x+1]
//     return recordTable->pack(myTuple1, 2);
// }}

souffle::RamDomain myappend(souffle::SymbolTable* symbolTable, souffle::RecordTable* recordTable, souffle::RamDomain arg) {{
    assert(symbolTable && "NULL symbol table");
    assert(recordTable && "NULL record table");
 
    if (arg == 0) {
        // Argument is nil
        souffle::RamDomain myTuple[2] = {0, 0};
        // Return [0, nil]
        return recordTable->pack(myTuple, 2);
    } else {
        // Argument is a list element [x, l] where
        // x is a number and l is another list element
        const souffle::RamDomain* myTuple0 = recordTable->unpack(arg, 2);
        souffle::RamDomain myTuple1[2] = {myTuple0[0] + 1, myTuple0[0]};
        // Return [x+1, [x, l]]
        return recordTable->pack(myTuple1, 2);
    }
}}

int32_t f(int32_t x) {
    return x + 1;
}

const char *g() {
    return "Hello world";
}
}

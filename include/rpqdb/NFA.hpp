#ifndef RPQDB_NFA_H
#define RPQDB_NFA_H

#include <vector>
#include <stack>
#include <string>
#include <utility>
#include <unordered_set>
#include <map>
#include <set>
#include <queue>
#include <iostream>
#include <memory>

namespace rpqdb {
	using namespace std;
	
	class State;
	
	typedef int StateID;
	
	// Transition structure to pair label with target state
	struct Transition {
		string label;
		State* target;
	};
	
	class State {
	public:
		const StateID id;         // Immutable unique identifier
		bool is_accepting;    	// More semantic than 'end'
		vector<Transition> transitions;
	
		explicit State(StateID state_id) 
			: id(state_id), is_accepting(false) {}
	};
	
	class NFA {
	private:
		vector<unique_ptr<State>> states;
		int state_counter = 0;  // Internal ID management
		int nfa_counter = 0;
		bool dirty = true; // Flag to track if the NFA has been modified
		unique_ptr<NFA> dfa; // Store the DFA representation
	
		// Private method to generate unique state IDs
		int generate_id() { 
			return state_counter++; 
		}
	
		// subset construction
		NFA toDFA() {
			NFA dfa;		
			map<set<State*>, State*> subset_to_dfa_state;
			
			// Get all possible transition labels from the NFA (excluding ε)
			set<string> all_labels;
			for (const auto& state : states) {
				for (const auto& trans : state->transitions) {
					if (!trans.label.empty()) {
						all_labels.insert(trans.label);
					}
				}
			}
	
			// Helper function to get next states for a given set of states and input
			auto label_closure = [](const set<State*>& states, const string& input) -> set<State*> {
				set<State*> result;
				for (State* s : states) {
					for (const auto& trans : s->transitions) {
						if (trans.label == input) {
							result.insert(trans.target);
						}
					}
				}
				return result;
			};
	
			// Start with ε-closure of initial state
			set<State*> initial = epsilon_closure({start_state});
			dfa.start_state = dfa.create_state();
			subset_to_dfa_state[initial] = dfa.start_state;
	
			// Check if initial state should be accepting
			for (State* s : initial) {
				if (s->is_accepting) {
					dfa.start_state->is_accepting = true;
					break;
				}
			}
	
			queue<set<State*>> worklist;
			worklist.push(initial);
	
			while (!worklist.empty()) {
				set<State*> current_subset = worklist.front();
				worklist.pop();
				State* current_dfa_state = subset_to_dfa_state[current_subset];
	
				// Process each possible input symbol
				for (const string& label : all_labels) {
					set<State*> next_states = label_closure(current_subset, label);
					if (next_states.empty()) continue;
					
					set<State*> next_subset = epsilon_closure(next_states);
					if (next_subset.empty()) continue;
	
					// Create new DFA state if needed
					if (subset_to_dfa_state.find(next_subset) == subset_to_dfa_state.end()) {
						State* new_state = dfa.create_state();
						subset_to_dfa_state[next_subset] = new_state;
						
						// Check if new state should be accepting
						for (State* s : next_subset) {
							if (s->is_accepting) {
								new_state->is_accepting = true;
								break;
							}
						}
						
						worklist.push(next_subset);
					}
	
					// Add transition
					dfa.add_transition(current_dfa_state, subset_to_dfa_state[next_subset], label);
				}
			}
			return dfa;
		}
	
	public:
		// Add these declarations
		NFA() = default;
		NFA(const NFA&) = delete;              // Delete copy constructor
		NFA& operator=(const NFA&) = delete;    // Delete copy assignment
		NFA(NFA&&) = default;                  // Enable move constructor
		NFA& operator=(NFA&&) = default;       // Enable move assignment
	
		State* start_state = nullptr;
		State* end_state = nullptr;
	
		// Creates and owns a new state
		State* create_state() {
			dirty = true;
			states.push_back(make_unique<State>(generate_id()));
			return states.back().get();
		}
	
		State* create_state(StateID id) {
			dirty = true;
			states.push_back(make_unique<State>(id));
			return states.back().get();
		}
	
		// Adds a transition between states
		void add_transition(State* from, State* to, const string& label) {
			dirty = true;
			from->transitions.push_back({label, to});
		}
	
		// Merges another NFA into this one (basic implementation)
		// rvalue reference to an NFA object that enables move semantics 
		// transfer resources (dynamically allocated memory) from one object to another
		void merge(NFA&& other) {
			dirty = true;
			for (auto& state : other.states) {
				states.push_back(std::move(state));
			}
		}
	
		// Helper function to get ε-closure
		set<State *> epsilon_closure (const set<State*>& states) {
			set<State*> closure = states;
			stack<State*> stack;
			for (State* s : states) stack.push(s);
	
			while (!stack.empty()) {
				State* current = stack.top();
				stack.pop();
	
				for (const auto& trans : current->transitions) {
					if (trans.label.empty() && closure.find(trans.target) == closure.end()) {
						closure.insert(trans.target);
						stack.push(trans.target);
					}
				}
			}
			return closure;
		};
	
		// Lazy computation
		// NFA & allows the caller to access the DFA, w/o transferring ownership 
		// NFA && allows the caller to steal/move the DFA
		NFA getDFA() {
			if (dirty || !dfa) {
				dfa = make_unique<NFA>(toDFA()); // Compute and store the DFA
				dirty = false; // Mark as clean
			}
			return std::move(*dfa);
		}
	
		// Apply to DFAs
		NFA product(NFA& other) {
			NFA result;
	
			// Map to store pairs of states and their corresponding new state in the product NFA
			using StatePair = std::pair<State*, State*>;
			map<StatePair, State*> state_map;
	
			// Helper function to get or create a new state in the product NFA
			auto get_or_create_state = [&](State* s1, State* s2) -> State* {
				StatePair key = {s1, s2};
				if (state_map.find(key) == state_map.end()) {
					State* new_state = result.create_state();
					state_map[key] = new_state;
					// Mark as accepting if both s1 and s2 are accepting
					if (s1->is_accepting && s2->is_accepting) {
						new_state->is_accepting = true;
					}
				}
				return state_map[key];
			};
	
			State* start1 = start_state;
			State* start2 = other.start_state;
			result.start_state = get_or_create_state(start1, start2);
	
			// Perform a breadth-first search (BFS) to explore all reachable state pairs
			queue<StatePair> queue;
			queue.push({start1, start2});
	
			while (!queue.empty()) {
				auto [current1, current2] = queue.front();
				queue.pop();
	
				// Get the corresponding state in the product NFA
				State* current_product_state = get_or_create_state(current1, current2);
	
				// Process transitions from both NFAs
				for (const auto& trans1 : current1->transitions) {
					for (const auto& trans2 : current2->transitions) {
						State* next1 = trans1.target;
						State* next2 = trans2.target;
						if (trans1.label == trans2.label) {
							State* next_product_state = get_or_create_state(next1, next2);
							result.add_transition(current_product_state, next_product_state, trans1.label);
							queue.push({next1, next2});
						}
					}
				}
			}
	
			return result;
		}
	
		// Helper to create basic pattern: "from --label--> to"
		static NFA create_basic(const string& label) {
			NFA nfa;
			State* start = nfa.create_state();
			State* end = nfa.create_state();
			end->is_accepting = true;
			
			nfa.add_transition(start, end, label);
			nfa.start_state = start;
			nfa.end_state = end;
			return nfa;
		}
	
		bool accepts(const std::string& input) const {
			if (!start_state) {
				return false; // No start state, cannot accept any string
			}
	
			// Use a queue to perform a breadth-first search (BFS) of the NFA
			// Each element in the queue is a pair: (current_state, current_position_in_input)
			using StatePositionPair = std::pair<State*, size_t>;
			std::queue<StatePositionPair> queue;
	
			// Start with the initial state and the beginning of the input
			queue.push({start_state, 0});
	
			while (!queue.empty()) {
				auto [current_state, current_position] = queue.front();
				queue.pop();
	
				// If we've processed the entire input and are in an accepting state, return true
				if (current_position == input.size() && current_state->is_accepting) {
					return true;
				}
	
				// Process ε-transitions (transitions with an empty label)
				for (const auto& transition : current_state->transitions) {
					if (transition.label.empty()) {
						// Move to the target state without consuming any input
						queue.push({transition.target, current_position});
					}
				}
	
				// If we haven't reached the end of the input, process transitions that match the current input symbol
				if (current_position < input.size()) {
					char current_symbol = input[current_position];
					for (const auto& transition : current_state->transitions) {
						if (!transition.label.empty() && transition.label[0] == current_symbol) {
							// Move to the target state and consume the current input symbol
							queue.push({transition.target, current_position + 1});
						}
					}
				}
			}
	
			// If no accepting state is reached, return false
			return false;
		}
	
		void print() {
			if (states.empty()) {
				cout << "NFA is empty.\n";
				return;
			}
	
			cout << "NFA Visualization:\n";
			cout << "[Starting] " << start_state << endl;
			for (const auto& state : states) {
				cout << "State " << state.get();
				if (state->is_accepting) {
					cout << " [Accepting]";
				}
				cout << ":\n";
	
				for (const auto& trans : state->transitions) {
					cout << "  --" << trans.label << "--> State " << trans.target << "\n";
				}
			}
		}
	};
	
	string re2post(const string& re) {
		int nalt = 0;  // Number of alternations
		int natom = 0; // Number of atoms
	
		string result; // Output string (replaces `buf`)
		result.reserve(re.size() * 2); // Reserve space for efficiency
	
		// Stack to track nested parentheses (replaces `paren` array)
		struct ParenState {
			int nalt;
			int natom;
		};
		stack<ParenState> parenStack;
		ParenState state; 
	
		for (char ch : re) {
			switch (ch) {
				case '(':
					if (natom > 1) {
						--natom;
						result += '.';
					}
					if (parenStack.size() >= 100) {
						throw runtime_error("Too many nested parentheses");
					}
					parenStack.push({nalt, natom});
					nalt = 0;
					natom = 0;
					break;
	
				case '|':
					if (natom == 0) {
						throw runtime_error("Invalid regex: alternation with no atoms");
					}
					while (--natom > 0) {
						result += '.';
					}
					nalt++;
					break;
	
				case ')':
					if (parenStack.empty()) {
						throw runtime_error("Invalid regex: unmatched parenthesis");
					}
					if (natom == 0) {
						throw runtime_error("Invalid regex: empty group");
					}
					while (--natom > 0) {
						result += '.';
					}
					for (; nalt > 0; nalt--) {
						result += '|';
					}
					state = parenStack.top();
					parenStack.pop();
					nalt = state.nalt;
					natom = state.natom;
					natom++;
					break;
	
				case '*':
				case '+':
				case '?':
					if (natom == 0) {
						throw runtime_error("Invalid regex: quantifier with no atom");
					}
					result += ch;
					break;
	
				default:
					if (natom > 1) {
						--natom;
						result += '.';
					}
					result += ch;
					natom++;
					break;
			}
		}
	
		if (!parenStack.empty()) {
			throw runtime_error("Invalid regex: unmatched parenthesis");
		}
		while (--natom > 0) {
			result += '.';
		}
		for (; nalt > 0; nalt--) {
			result += '|';
		}
	
		return result;
	}
	
	NFA post2nfa(const string& postfix) {
		if (postfix.empty()) {
			throw runtime_error("Empty postfix expression");
		}
	
		stack<NFA> nfa_stack;
		
		for (char ch : postfix) {
			switch (ch) {
				case '.': {
					if (nfa_stack.size() < 2) {
						throw runtime_error("Invalid postfix expression: insufficient operands for concatenation");
					}
					NFA nfa2 = std::move(nfa_stack.top());
					nfa_stack.pop();
					NFA nfa1 = std::move(nfa_stack.top());
					nfa_stack.pop();
					nfa1.end_state->is_accepting = false;
					nfa1.add_transition(nfa1.end_state, nfa2.start_state, ""); // ε-transition
					nfa1.merge(std::move(nfa2));
					nfa1.end_state = nfa2.end_state;
					// nfa1.print();
					nfa_stack.push(std::move(nfa1));
					break;
				}
	
				// Alternation (|)
				case '|': {
					if (nfa_stack.size() < 2) {
						throw runtime_error("Invalid postfix expression: insufficient operands for alternation");
					}
					NFA nfa2 = std::move(nfa_stack.top());
					nfa_stack.pop();
					NFA nfa1 = std::move(nfa_stack.top());
					nfa_stack.pop();
	
					// Create a new NFA for the alternation
					NFA result;
					State * start = result.create_state();
					State * end = result.create_state();
					end->is_accepting = true;
	
					// Add ε-transitions from the new start state to the start states of nfa1 and nfa2
					result.add_transition(start, nfa1.start_state, "");
					result.add_transition(start, nfa2.start_state, "");
	
					result.merge(std::move(nfa1));
					result.merge(std::move(nfa2));
	
					// Add ε-transitions from the end states of nfa1 and nfa2 to the new end state
					nfa1.end_state->is_accepting = false;
					nfa2.end_state->is_accepting = false;
					result.add_transition(nfa1.end_state, end, "");
					result.add_transition(nfa2.end_state, end, "");
	
					// Set the new start and end states
					result.start_state = start;
					result.end_state = end;
	
					nfa_stack.push(std::move(result));
					break;
				}
	
				// Kleene star (*)
				case '*': {
					if (nfa_stack.empty()) {
						throw runtime_error("Invalid postfix expression: insufficient operands for Kleene star");
					}
					NFA nfa1 = std::move(nfa_stack.top());
					nfa_stack.pop();
					NFA result;
					// Create a new NFA for the Kleene star
					State* start = result.create_state();
					State* end = result.create_state();
					end->is_accepting = true;
	
					result.merge(std::move(nfa1));
	
					// Add ε-transitions for the Kleene star
					result.add_transition(start, end, ""); // ε-transition for zero repetitions
					result.add_transition(start, nfa1.start_state, ""); // ε-transition to the NFA
					nfa1.end_state->is_accepting = false;
					result.add_transition(nfa1.end_state, end, ""); // ε-transition from the NFA to the end state
					result.add_transition(nfa1.end_state, nfa1.start_state, ""); // ε-transition for repetition
	
					// Set the new start and end states
					result.start_state = start;
					result.end_state = end;
	
					nfa_stack.push(std::move(result));
					break;
				}
	
				// Default case: single character
				default: {
					NFA nfa = NFA::create_basic(string(1, ch));
					// cout << "Debug: create a new basic block with id " << nfa.start_state << endl;
					nfa_stack.push(std::move(nfa));
					break;
				}
			}
		}
	
		if (nfa_stack.size() != 1) {
			cout << "Debug: total operands is " << nfa_stack.size() << endl;
			throw runtime_error("Invalid postfix expression: too many operands");
		}
	
		NFA result = std::move(nfa_stack.top());
		nfa_stack.pop();  // Don't forget to pop!
		return result;
	}
} // namespace rpqdb

#endif
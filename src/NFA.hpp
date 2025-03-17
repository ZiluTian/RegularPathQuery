#include <vector>
#include <stack>
#include <string>
#include <utility>
#include <unordered_set>

using namespace std;

// See https://swtch.com/~rsc/regexp/regexp1.html
/*
 * Convert infix regexp re to postfix notation.
 * Insert . as explicit concatenation operator.
 */

class State;

// typedef pair<int, int> StateID;
typedef int StateID;

// Transition structure to pair label with target state
struct Transition {
	string label;
	State* target;
};

class State {
public:
	const StateID id;         // Immutable unique identifier
	bool is_accepting;    // More semantic than 'end'
	vector<Transition> transitions;

	explicit State(StateID state_id) 
		: id(state_id), is_accepting(false) {}
};

class NFA {
private:
	vector<unique_ptr<State>> states;
	int state_counter = 0;  // Internal ID management
	int nfa_counter = 0;

	// Private method to generate unique state IDs
	int generate_id() { 
		return state_counter++; 
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
		states.push_back(make_unique<State>(generate_id()));
		return states.back().get();
	}

	// Adds a transition between states
	void add_transition(State* from, State* to, const string& label) {
		from->transitions.push_back({label, to});
	}

	// Merges another NFA into this one (basic implementation)
	// rvalue reference to an NFA object that enables move semantics 
	// transfer resources (dynamically allocated memory) from one object to another
	void merge(NFA&& other) {
		for (auto& state : other.states) {
			states.push_back(std::move(state));
		}
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

	void print() {
        if (states.empty()) {
            cout << "NFA is empty.\n";
            return;
        }

        cout << "NFA Visualization:\n";
        for (const auto& state : states) {
            cout << "State " << state;
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
				cout << "Debug: create a new basic block with id " << nfa.start_state << endl;
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
import os

# Define a Generator class with a function generate
class Generator:
    def check_file_exists(self):
        if os.path.exists(self.filename):
            print(f"File {self.filename} already exists")
            exit()

    def generate(self):
        pass

class CounterGenerator(Generator):
    def __init__(self, number_of_nodes, filename):
        self.number_of_nodes = number_of_nodes
        self.filename = filename
        self.check_file_exists()

    def generate(self):
        f = open(self.filename, "w")
        for i in range(self.number_of_nodes):
            f.write(f"{i}	0\n")
        f.close()

# Define a graph generator class that inherits from Generator, with constructor that takes a list of layer sizes
class ForwardGraphGenerator(Generator):
    def __init__(self, layers, connectivity, filename):
        assert len(layers) >= 3, "The number of layers should be at least 3"
        self.layers = layers
        self.connectivity = connectivity
        self.filename = filename

        self.check_file_exists()

    def generate(self):
        # replace references to P with self.layers
        S = self.layers[0]
        T = self.layers[-1]

        total_edges = 0
        for i in range(1, len(self.layers)):
            total_edges += self.layers[i] * self.layers[i-1]

        print(f"Generating a {len(self.layers)}-layer graph with {S} source nodes, {T} target nodes, {sum(self.layers)} nodes, and {total_edges} edges")

        f = open(self.filename, "w")

        total_nodes = sum(self.layers)
        prefix_sum = [0]
        for p in self.layers:
            prefix_sum.append(prefix_sum[-1] + p)

        # Add a self-loop with label 1on the source nodes
        for i in range(S):
            f.write(f"{i}	1	{i}\n")

        # Add a self-loop with label 3 on the target nodes
        for i in range(total_nodes - T, total_nodes):
            f.write(f"{i}	3	{i}\n")

        # Add edges with label 2 between the intermediate nodes
        for i in range(1, len(prefix_sum)-1):
            for j in range(prefix_sum[i-1], prefix_sum[i]):
                for k in range(prefix_sum[i], prefix_sum[i+1]):
                    f.write(f"{j}	2	{k}\n")
        f.close()


class LimitGenerator(Generator):
    def __init__(self, input_graph_filename, bound, number_of_nodes, filename):
        self.input_graph_filename = input_graph_filename
        self.bound = bound
        self.number_of_nodes = number_of_nodes
        self.filename = filename
        self.check_file_exists()

    def generate(self):
        output_rules = f"""
.decl labelled_edge(x:number, l:number, y:number)
.input labelled_edge(filename="{self.input_graph_filename}")
        """

        for i in range(self.number_of_nodes):
            output_rules += f"""
.decl bc_reachable{i}(y:number)
.output bc_reachable{i}(filename="out/out_bc_reachable_with_limit_{i}")

bc_reachable{i}(y) :- labelled_edge({i}, 3, y).
.limitsize bc_reachable{i}(n={self.bound})

.decl deg{i}(d:number)
.decl bc_light{i}(y:number)
.decl bc_heavy{i}(y:number)
.decl Q_light{i}(y:number)
.decl Q_heavy{i}(y:number)
.decl T{i}(y:number)

deg{i}(count : {{ bc_reachable{i}(_) }}) :- bc_reachable{i}(_).
.output deg{i}(filename="out/deg_with_limit_{i}")

bc_light{i}(y) :- bc_reachable{i}(y), deg{i}(d), d < {self.bound}.
bc_heavy{i}(y) :- bc_reachable{i}(y), deg{i}(d), d = {self.bound}.
        """

        for i in range(self.number_of_nodes):
            for z in range(self.number_of_nodes):
                output_rules += f"""
bc_reachable{i}(y) :- labelled_edge({i}, 2, {z}), bc_reachable{z}(y).
Q_light{i}(y) :- labelled_edge({i}, 1, {z}), bc_light{z}(y).
T{i}(y) :- labelled_edge({i}, 1, {z}), bc_heavy{z}(y).
T{i}(y) :- T{i}({z}), labelled_edge({z}, 2, y).
Q_heavy{i}(y) :- T{i}({z}), labelled_edge({z}, 3, y).
"""
        f = open(self.filename, "w")
        f.write(output_rules)
        f.close()


if (__name__ == "__main__"):
    # Create a generator
    # ForwardGraphGenerator([1, 15, 1], 1, "graph_deg1.facts").generate()
    # ForwardGraphGenerator([10, 50, 50, 100], 1, "graph_deg100.facts").generate()
    # Generate the graph
    # CounterGenerator(210, "counter.facts").generate()
    # LimitGenerator("graph_deg1.facts", 10, 17, "synDeg1_17n.dl").generate()
    LimitGenerator("graph_deg100.facts", 80, 210, "synDeg100_210n.dl").generate()
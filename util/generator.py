import os
import random
from collections import defaultdict

# Define a Generator class with a function generate
class Generator:
    def check_outfile_exists():
        if os.path.exists(self.out_file):
            print(f"File {self.out_file} already exists")
            exit()

    def generate(self):
        pass

class BoundedBCReachableGenerator(Generator):
    def __init__(self, edge_file, out_file, bound):
        if not os.path.exists(edge_file):
            print(f"Edge file {edge_file} does not exist!")
            exit()

        self.c_edges = defaultdict(set)
        self.b_edges = defaultdict(set)
        # An auxiliary data structure to speed up "for vertex x, find all vertices w that E(w, 2, x)"
        self.reverse_b_edges = defaultdict(set)

        self.bound = bound
        # Use an aux data structure to track the size of bc_reachable edges, to avoid computations
        self.bc_reachable_size = defaultdict(int)
        self.out_file = out_file

        with open(edge_file, "r") as file:
            while True:
                line = file.readline()
                if not line:
                    break
                v1, label, v2 = map(int, line.split())
                if (label == 3):
                    if v1 in self.c_edges: 
                        if (self.bc_reachable_size[v1] < bound) and (v2 not in self.c_edges[v1]):
                            self.c_edges[v1].add(v2)
                            self.bc_reachable_size[v1] += 1
                    else:
                        self.c_edges[v1].add(v2)
                        self.bc_reachable_size[v1] = 1
                elif (label == 2):
                    self.b_edges[v1].add(v2)
                    self.reverse_b_edges[v2].add(v1)

    # A generator that returns a new vertex to be added, if any
    def new_vertex_generator(self):
        for x in self.b_edges:
            if self.bc_reachable_size[x] < self.bound:
                for z in self.b_edges[x]:
                    for y in self.bc_reachable[z]:
                        if y not in self.bc_reachable[x]:
                            self.bc_reachable[x].add(y)
                            self.bc_reachable_size[x] += 1
                            yield (x, y)

    def generate(self):
        self.bc_reachable = self.c_edges
        vertex_gen = self.new_vertex_generator()

        try:
            new_vertex = next(vertex_gen)
            while new_vertex:
                x, y = new_vertex
                # for w in self.b_edges:
                #     if (x in self.b_edges[w]):
                for w in self.reverse_b_edges[x]:
                    if (y not in self.bc_reachable[w] and self.bc_reachable_size[w] < self.bound):
                        self.bc_reachable[w].add(y)
                        self.bc_reachable_size[w] += 1
                new_vertex = next(vertex_gen)
        except StopIteration as e:
            print("No more new vertex to be added")

        f = open(self.out_file, "w")
        for i in self.bc_reachable:
            for j in self.bc_reachable[i]:
                f.write(f"{i}	{j}\n")
        f.close()

class CounterGenerator(Generator):
    def __init__(self, number_of_nodes, filename):
        self.number_of_nodes = number_of_nodes
        self.check_outfile_exists()
        self.out_file = filename

    def generate(self):
        f = open(self.out_file, "w")
        for i in range(self.number_of_nodes):
            f.write(f"{i}	0\n")
        f.close()

# Define a graph generator class that inherits from Generator, with constructor that takes a list of layer sizes
class ForwardGraphGenerator(Generator):
    def __init__(self, layers, connectivity, filename):
        assert len(layers) >= 3, "The number of layers should be at least 3"
        self.layers = layers
        self.connectivity = connectivity
        self.out_file = filename
        self.check_outfile_exists()

    def generate(self):
        # replace references to P with self.layers
        S = self.layers[0]
        T = self.layers[-1]

        print(f"Generating a {len(self.layers)}-layer graph with {S} source nodes, {T} target nodes, and {sum(self.layers)} nodes")

        f = open(self.out_file, "w")

        total_nodes = sum(self.layers)
        prefix_sum = [0]
        for p in self.layers:
            prefix_sum.append(prefix_sum[-1] + p)

        # Add a self-loop with label 1 on the source nodes
        for i in range(S):
            f.write(f"{i}	1	{i}\n")

        # Add a self-loop with label 3 on the target nodes
        for i in range(total_nodes - T, total_nodes):
            f.write(f"{i}	3	{i}\n")

        # Add edges with label 2 between the intermediate nodes
        for i in range(1, len(prefix_sum)-1):
            for j in range(prefix_sum[i-1], prefix_sum[i]):
                for k in range(prefix_sum[i], prefix_sum[i+1]):
                    if random.random() < self.connectivity:
                        f.write(f"{j}	2	{k}\n")
        f.close()


class LimitGenerator(Generator):
    def __init__(self, input_graph_filename, bound, number_of_nodes, filename):
        self.input_graph_filename = input_graph_filename
        self.bound = bound
        self.number_of_nodes = number_of_nodes
        self.out_file = filename
        self.check_outfile_exists()

    def generate(self):
        output_rules = f"""
.decl labelled_edge(x:number, l:number, y:number)
.input labelled_edge(filename="{self.input_graph_filename}")
        """

        for i in range(self.number_of_nodes):
            output_rules += f"""
.decl bc_reachable{i}(y:number)
.output bc_reachable{i}(filename="dbg/out_bc_reachable_with_limit_{i}")

bc_reachable{i}(y) :- labelled_edge({i}, 3, y).
.limitsize bc_reachable{i}(n={self.bound})

.decl deg{i}(d:number)
.decl bc_light{i}(y:number)
.decl bc_heavy{i}(y:number)
.decl Q_light{i}(y:number)
.decl Q_heavy{i}(y:number)
.decl T{i}(y:number)

deg{i}(count : {{ bc_reachable{i}(_) }}) :- bc_reachable{i}(_).
.output deg{i}(filename="dbg/deg_with_limit_{i}")

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
        f = open(self.out_file, "w")
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
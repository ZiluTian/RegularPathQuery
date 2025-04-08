import sys
import os
import json

save_dir = "resources/"

def gen_edges(vertex, labels):
    ans = ""
    for l in labels:
        ans += f"{vertex} {l} {vertex+1}\n"
    return ans

# path where all edges labelled with b (b*)
def gen_path_bs(graph_size, file_name):
    vid = 1
    with open(file_name, "w") as file:
        print("Write to " + file_name)
        while (vid < graph_size):
            file.write(f"{vid} b {vid+1}\n")
            vid += 1

# path where all edges labelled with b except the last one (b*c)
def gen_path_bsc(graph_size, file_name):
    vid = 1
    with open(file_name, "w") as file:
        print("Write to " + file_name)
        while (vid < graph_size-1):
            file.write(f"{vid} b {vid+1}\n")
            vid += 1
        file.write(f"{vid} c {vid+1}")

# fully connected neural network
# nodes in the first layer are associated with a self-loop labelled with a
# nodes in the last layer are associated with a self-loop labelled with c
# edges connecting layers are labelled with b
def gen_cnn(layers, file_name):
    S = layers[0]
    T = layers[-1]
    total_nodes = sum(layers)

    print(f"Write to {file_name} a {len(layers)}-layer graph with {S} source nodes, {T} target nodes, and {total_nodes} nodes")

    with open(file_name, "w") as f:
        prefix_sum = [0]
        for p in layers:
            prefix_sum.append(prefix_sum[-1] + p)

        # Add a self-loop with label a on the source nodes
        for i in range(S):
            f.write(f"{i} a {i}\n")

        # Add a self-loop with label b on the target nodes
        for i in range(total_nodes - T, total_nodes):
            f.write(f"{i} c {i}\n")

        # Add edges with label b between intermediate nodes
        for i in range(1, len(prefix_sum)-1):
            for j in range(prefix_sum[i-1], prefix_sum[i]):
                for k in range(prefix_sum[i], prefix_sum[i+1]):
                    f.write(f"{j} b {k}\n")

def gen_disjoint_cycles(cycle_size, num_cycles, file_name):
    labels = ["b", "c"]

    with open(file_name, "w") as file:
        for i in range(num_cycles):
            init_vid = 1 + i * cycle_size 
            for i in range(init_vid, init_vid + cycle_size-1):
                file.write(gen_edges(i, labels))
            # Connect the last element to head
            for l in labels:
                file.write(f"{init_vid + cycle_size - 1} {l} {init_vid}\n")

if (__name__ == "__main__"):
        # Get user input
        if len(sys.argv) > 1:
            config_file = sys.argv[1]

            f = open(f"conf/{config_file}.json")
            config = json.load(f)
            graph_file_name = config["output_graph"]
            query = config["query"]
            print(f"The graph will be saved as {graph_file_name}")            

            if os.path.exists(graph_file_name):
                print(f"Graph file {graph_file_name} exists!")
                exit()
            
            graph_generator = config["generator"]
            if graph_generator in globals() and callable(globals()[graph_generator]):
                func = globals()[graph_generator]
                result = func(*[config[arg] for arg in config["gen_args"]])

        else:
            print("No input provided. Please pass the graph type, size, and file name as command-line arguments.")
            exit()
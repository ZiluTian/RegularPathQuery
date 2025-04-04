import sys
import os

# Graph for Example 6.1
# Graph for Example 6.2
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
            graph_type = int(sys.argv[1])
            graph_size = int(sys.argv[2])
            if (graph_type == 1):
                graph_file_name = save_dir+f"path_{graph_size}.txt"
            elif (graph_type == 2):
                graph_file_name = save_dir+f"disjoint_cycles_{graph_size}.txt"
            elif (graph_type == 3):
                graph_file_name = save_dir+f"pathbsc_{graph_size}.txt"

            print(f"The graph will be saved as {graph_file_name}")            
            
            if os.path.exists(graph_file_name):
                print(f"Graph file {graph_file_name} exists!")
                exit()
            else:
                if (graph_type == 1):
                    gen_path_bs(graph_size, graph_file_name)
                elif (graph_type == 2):
                    gen_disjoint_cycles(graph_size, 2, graph_file_name)
                elif (graph_type == 3):
                    gen_path_bsc(graph_size, graph_file_name)

        else:
            print("No input provided. Please pass the graph type, size, and file name as command-line arguments.")
            exit()
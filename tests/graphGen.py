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

def generate(graph_type, graph_size, file_name):
    vid = 1
    labels = ["a", "b"]
    if graph_type == 2:
        labels = ["b", "c"]
    elif not graph_type == 1:
        print("Unrecognized graph type!")
        return
    
    with open(file_name, "w") as file:
        print("Write to " + file_name)
        while (vid < graph_size):
            file.write(gen_edges(vid, labels))
            vid += 1
        # Connect the last tail element with the head
        for l in labels:
            file.write(f"{vid} {l} 1\n")

if (__name__ == "__main__"):
        # Get user input
        if len(sys.argv) > 1:
            graph_type = int(sys.argv[1])
            graph_size = int(sys.argv[2])
            graph_file_name = f"graph_tc_{graph_type}_{graph_size}.txt"
            print(f"The graph will be saved as {graph_file_name} in resources/")            
            
            if os.path.exists(save_dir+graph_file_name):
                print(f"Graph file {graph_file_name} exists in {save_dir}!")
                exit()
            else:
                generate(graph_type, graph_size, save_dir + graph_file_name)
        else:
            print("No input provided. Please pass the graph type, size, and file name as command-line arguments.")
            exit()
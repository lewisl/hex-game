// 
// implement class Graph to hold graph definition and create graphs
// implement Dijkstra's shortest path algorith as a class with
//     nodes, shortest paths, and path costs as members
//
// Run a simulation with 2 passes with graphs of 50 nodes and densities of 0.2 and 0.4
// Must be compiled using c++11 as in: g++ -std=c++11 shortest_path_v6.cpp -o shortest_path_v6
// To run with the class assignment inputs just run without inputs as `shortest_path_v6`
// Programmer: Lewis Levin
// Date: January, 2023
//

#include <iostream>
#include <random>
#include <vector>
#include <deque>            // sequence of nodes in a path between start and destination
#include <unordered_map>    // container for definition of Graph, costs, previous nodes for Dijkstra
#include <set>              // hold nodes for shortest path algorithm
#include <fstream>          // to write graph to file and read graph from file
#include <sstream>          // to use stringstream to parse lines

using namespace std;

// setup data for nodes (node traits like color or type, etc.)

struct Data
{
    int val;
};
    // in main
        //  const bool use_data=true;
        //  const vector<int> allowed_data {0,1,2};
        //  const vector<int> select_data {1};

// holds an edge for a starting node (aka "edge"): neighbor node, cost to it
     // doesn't include source node because that is part of the outer data structure, class Graph
struct Edge 
{  
    int to_node;   // it's ok that these are public: this is just a trivial struct
    int cost;

    Edge(int to_node=0, int cost=0) : to_node(to_node), cost(cost) {}
};

// output an Edge in an output stream
ostream& operator<< (ostream& os, const Edge& e)
{
    os << "  to: " << e.to_node << " cost: " << e.cost << endl;
    return os;
}

// Random number generation
mt19937 rng(time(0));  //  mt19937 = Mersenne twister is better and faster than default_random_engine

    // as a probability value 
    uniform_real_distribution<float> probability(0.0, 1.0);
    // use as probability(rng) => returns a value in [0.0, 1.0]
    
    // as a selection from a vector, ex: vector<int> allowed_data = {0,1,2};
    // use as const int n_values = allowed_data.size(); allowed_data[rng() % n_values]; 
        //  => returns one of the vector elements in allowed_data

// other functions

// test if a value is in a set
bool is_in(int x, set<int> v_set)
{
    bool ret = false;
    for (int v : v_set)
        {
            if (x == v) {
                ret = true;
                break;
            }
        }
    return ret;
}


// ##########################################################################
// #             class Graph
// ##########################################################################
class Graph 
{
private:
    unordered_map<int, vector<Edge>> graph;  
    unordered_map<int, Data> node_data;   // holds Data values of all nodes
    
public:
    set<int> all_nodes;    // maintains sorted list of nodes
    bool has_node_data;
    
    // effectively, constructors
    void make_random_graph(int, float, bool, vector<int>, float, float);
    void load_graph_from_file(string);
    
    // other methods
    int size = graph.size();
    void write_graph(string);
    void print_graph(ostream&);
    int count_nodes();
    int count_edges();
    vector<Edge> get_neighbors(int);  
    void add_edge(int, int, int);

    int getsize()    // get size in bytes
    {
        int ret = 0;
        ret += sizeof(graph);
        for (const auto & ve : graph) {
            ret += sizeof(ve);
            for (const auto & e : ve.second) {
                ret += sizeof(e);
            }
        }
        return ret;
    }

    unordered_map<int, vector<Edge>>::iterator begin()
    {
        return graph.begin();
    }

    unordered_map<int, vector<Edge>>::iterator end()
    {
        return graph.end();
    }
    
    int get_node_data(int node)
    {
        // TODO should assert that graph has node data, else caller must check!
        return node_data[node].val;  // TODO this is really fragile
    }

};  // end of class Graph


// make a graph with randomly placed edges with random cost
void Graph::make_random_graph(int num, float density, 
    bool use_data=false, vector<int> allowed_data={},
    float min_cost=1.0, float max_cost=10.0) 
{
    graph.reserve(num);  // request required number of elements to reduce rehashes and resizes
    has_node_data = use_data;
    // graph.max_load_factor(0.5);

    int to_node;
    int cost;
    float cost_diff = max_cost - min_cost;

    // add nodes
    for (int i = 0; i < num; i++) {
        graph[i] = vector<Edge>();
        all_nodes.insert(i);   
    }

    // add edges
    for (int i = 0; i < num; i++) {
        for (int j = i+1; j < num; j++) {
            to_node = probability(rng) < density ? j : 0;
            if (to_node != 0) {
                cost = static_cast<int>(min_cost + probability(rng) * cost_diff + 0.5);
                add_edge(i, j, cost);  // adds edge in both directions for non-directional graph
            }
        }
    }
    
    // add costs
    for (int i = 0; i < num; i++) {
        for (int j = i+1; j < num; j++) {
            to_node = probability(rng) < density ? j : 0;
            if (to_node != 0) {
                cost = static_cast<int>(min_cost + probability(rng) * cost_diff + 0.5);
                add_edge(i, j, cost);  // adds edge in both direction for non-directional graph
            }
        }
    }
    
    // add traits (data values for each node)
    if (use_data)
    {
        node_data.reserve(num);
        int n_data = allowed_data.size();
        for (int i = 0; i < num; i++) {
            node_data[i].val = allowed_data[rng() % n_data];
        }
    }
    
}


// 
//     Read a graph file to initialize input graph using the format of the example:
//
//     node 1       // node must be positive integer; don't need to be consecutive
//         edge 2 3  // edge is to_node, cost. No error checking so to_node must exist
//     node 2
//         edge 1 4
//         edge 3 5
//     node 3       // no edges so this is a dead-end
// 
void Graph::load_graph_from_file(string filename)
{
    // prepare input file
    ifstream infile;
    infile.open(filename);  // open a file to perform read operation using file object
    if (!(infile.is_open())) {
        cout << "Error opening file: " << filename << " Terminating.\n";
        exit(-1);
    }

    // read the file line by line and create graph
    string linestr, leader;
    int node_id, to_node;
    int cost;

    while (getline(infile, linestr))
    {
        stringstream ss {linestr};
        ss >> leader;

        if (leader == "node") {
                ss >> node_id;
                graph[node_id] = vector<Edge>();  // create node with empty vector of Edges
                all_nodes.insert(node_id);
            }   
        else if (leader == "edge") {
            ss >> to_node >> cost;
            graph[node_id].push_back(Edge(to_node, cost));  // push Edge to current node_id
            }     
    }
} 


// 
//     Write current graph to text file using the format of the example:
//
//     node 1       // node must be positive integer; don't need to be consecutive
//         edge 2 3  // edge is to_node, cost. No error checking so to_node must exist
//     node 2
//         edge 1 4
//         edge 3 5
//     node 3       // no edge so this is a dead-end
// 
void Graph::write_graph(string filename)
{
    // prepare output file
    fstream outfile;
    outfile.open(filename, ios::out);  // open a file to perform write operation using file object
    if (!(outfile.is_open())) {
        cout << "Error opening file: " << filename << " Terminating.\n";
        exit(-1);
    }
    else {
        int node_id;
        vector<Edge> * edges;
        vector<int> keys;

        for (const int node_id : all_nodes) {
            edges = &(graph.at(node_id));
            outfile << "node " << node_id << endl;
            for (int j = 0; j < (*edges).size(); j++) {
                outfile << "    " << "edge " << 
                    edges->at(j).to_node << " " << edges->at(j).cost << endl;
            }
        }
    }
}


// print a graph to an output stream ot
//    you can pass an fstream as the output stream
//    cout is the default value of ot--to screen/console
void Graph::print_graph(ostream& ot=cout)
{
    vector<int> keys;
    int node_id;
    vector<Edge> * edges;

    // print the graph in order
    ot << "\nSize of graph: " << graph.size() << " nodes.\n";
    for (const int node_id : all_nodes) {
        edges = &(graph.at(node_id));
        ot << "node " << node_id << endl;
        if (has_node_data) {
            ot << "    data: " << node_data[node_id].val << "\n";
        }
        for (int j = 0; j< (*edges).size(); j++) {       
            ot << "    " << "edge " << 
                edges->at(j).to_node << " " <<  edges->at(j).cost << endl;
        }
    }
}


// count the nodes in a graph
int Graph::count_nodes() 
{
    return graph.size();
}


// count the edges in a graph
int Graph::count_edges() 
{
    int sum=0;
    for (auto &node : graph) {    // 'for each' form of for loop
            sum += node.second.size();  // node is a pair<int, vector<edge>>
    }

    return sum/2;   // divide by 2 for non-directional graph  TODO: make non-directional v directional explicit
}


// get the neighbors of a node and as an edge: the node and the cost
vector<Edge> Graph::get_neighbors(int current_node)  
{
    return graph[current_node];  //the value type of graph is vector<Edge> and holds the neighbors
}


// add an edge to the graph
//      TODO add explicit control for non-directional or directional
void Graph::add_edge(int x, int y, int cost)
{
    if (graph.find(x) != graph.end()) {
        if (graph.find(y) != graph.end()) {
            for (auto edge : graph[x]) {    // iterate over edges of x
                if (y == edge.to_node) {
                    return;                 // to_node y already exists as an edge of x
                }
            }
            graph[x].push_back(Edge(y, cost));  //non-directional node: add directional in both ways
            graph[y].push_back(Edge(x, cost));

        }
        else {
            cout << "node " << y << " not found in graph.";
        }
    }
    else {
        cout << "node " << x << " not found in graph.";
    }
}
// ##########################################################################
// #             end of class Graph methods
// ##########################################################################




// ##########################################################################
// #             class Dijkstra
// ##########################################################################
class Dijkstra
{
private:
    set<int> path_nodes;
    unordered_map<int, int> path_costs;  // dest. node -> cost to reach 
    unordered_map<int, deque<int>> path_sequences;  // dest. node -> path of nodes to it
    
public:
    Dijkstra(Graph &, int, set<int>);   // no default constructor. arg 1 is starting node.
    float average_cost();
    int start_node;
    
    // print all shortest paths start_node to all other nodes
    friend  ostream& operator<< (ostream& os, Dijkstra & dp)
    {
        for (int node : dp.path_nodes) {
            if (node == dp.start_node) continue;  // don't print path from start_node to start_node
            os << "||     Path to " << node << "     ||\n";
            os << "  cost: " << dp.path_costs[node] << "\n";
            os << "  sequence: " << "[ ";
                for (auto &seq : dp.path_sequences[node]){
                    os << seq << " ";
                }
            os << "]" << endl;
        }
        return os;
    }

};


Dijkstra::Dijkstra(Graph &graf, int start_node, set<int> select_data)
{
    int num_nodes = graf.count_nodes();

    this->start_node = start_node;
    
    const int inf = INT_MAX;
    int neighbor_node=0;
    int current_node=0;
    int prev_node=0;
    int min = inf;
    int tmp_cost = 0;
    int node_val = -1;
    deque<int> tmpsequence; 
    vector<Edge>  neighbors;   // end_node and cost for each neighbor node

    // initial values for class members
    path_costs.reserve(num_nodes);
    path_sequences.reserve(num_nodes);
    
    // initialization for loop
    unordered_map<int, int> previous;
        previous.reserve(num_nodes);
    set<int> candidate_nodes = graf.all_nodes; // copy into set of candidates: the "open" list
    //  candidate_nodes.erase(start_node);
    
    // if using data, eliminate nodes that don't match data selection criteria from candidate list
    if (graf.has_node_data) {
        for (auto node : graf.all_nodes) {    // iterate nodes #2 through end
            node_val = graf.get_node_data(node);
            if (!is_in(node_val, select_data)) {
                candidate_nodes.erase(node);   // eliminate from candidate nodes
            }
        }
        graf.all_nodes.insert(start_node);
    }
    
    //  cout << "remaining count of items in candidate_nodes: " << candidate_nodes.size() << endl;

        
    for (auto current_node : graf.all_nodes) {
        path_costs[current_node] = inf;
        previous[current_node] = inf;
    }

    path_costs[start_node] = 0;
    previous[start_node] = 0;

    // algorithm loop
    while (!(candidate_nodes.empty()))
    {
        // current_node begins as start_node

        neighbors = graf.get_neighbors(current_node);  // vector<Edge>
        for (auto & neighbor : neighbors) {    // neighbor is an Edge
            neighbor_node = neighbor.to_node;
            // update the path_costs for neighbors of the current_node
            tmp_cost = path_costs[current_node] + neighbor.cost;
            if (tmp_cost < path_costs[neighbor_node]) {
                path_costs[neighbor_node] = tmp_cost;
                previous[neighbor_node] = current_node;
            }
        }
        path_nodes.insert(current_node);
        candidate_nodes.erase(current_node);  // we won't look at this node again

        // pick next current_node based on minimum distance, which will be a neighbor whose cost
            // has been updated from infinity to the network edge cost
        min = inf; 
        for (const int node : candidate_nodes) {  // always O(n)
            if (path_costs[node] < min) {
                min = path_costs[node];
                current_node = node;
            }
        }

        if (min == inf) {
            current_node = *candidate_nodes.begin();    // arbitrarily pick a node
        }

        if (path_costs[current_node] == inf) break;  // path to this node hasn't been found
    }

    // build sequences by walking backwards from each dest. node to start_node 
    for (const auto &walk_node : path_nodes)
    {
        prev_node = walk_node;
        do {
            tmpsequence.push_front(prev_node);  // it's a deque
            prev_node = previous[prev_node];
        } while (previous[prev_node] != start_node);
        tmpsequence.push_front(start_node);
        path_sequences[walk_node] = tmpsequence;

        tmpsequence.clear();  // IMPORTANT: clear before reusing for a new sequence!
    }
}


float Dijkstra::average_cost()
{
    int sum = 0;
    
    for (auto node : path_nodes) {
//      cout << "current cost: " << path_costs[node] << endl;
        sum += path_costs[node];
    }
    
    return (static_cast<float>(sum)) / (path_nodes.size() - 1); 
}
// ##########################################################################
// #             end of class Dijkstra methods
// ##########################################################################


// outputs from the simulation:  allows multiple output variables from function
struct Sim_outputs {    // critics: note this IS a class: see the constructor method
        float avg_path_length;
        float density;
        int graph_size;
        float edges_avg;
        int num_trials;

        // default constructor with no inputs; also works as by item constructor
        Sim_outputs(float apl=0, float density= 0.0, int size = 0,
                int edges_avg=0, int nt=0)  :
                avg_path_length(apl), density(density), graph_size(size),
                edges_avg(edges_avg), num_trials(nt) {}
    };
                  

void print_sim_output(ostream& ot, const Sim_outputs & out)
    {
        ot << "=========================================\n";
        ot << "Simulation of shortest path with: \n" 
           << "Graph Size: " << out.graph_size << "  Edges: " << out.edges_avg << "\n"
           << "Density: " << out.density  << "    Trials: " << out.num_trials << endl;
        ot << "Average path length = " << out.avg_path_length << "\n";
    }


// sorts by destination node and prints the paths from starting_node to each node


Sim_outputs run_simulation(int start_node, int num_trials, int size, float density, set<int> select_data)
    {
        Graph graf;  

        float sim_avg=0.0, edges_avg=0;
        float sum_for_avg=0.0;
        float sum_for_edges=0.0;

        for (int i=0; i < num_trials; i++) {
            graf.make_random_graph(size, density); 
            auto shortest_paths = Dijkstra(graf, start_node, select_data);
            sum_for_avg += shortest_paths.average_cost();
            sum_for_edges += graf.count_edges();
        }

        sim_avg = sum_for_avg / num_trials;
        edges_avg = sum_for_edges / num_trials;

        Sim_outputs outputs(sim_avg, density, size, edges_avg, num_trials);
        return outputs;
    }



int main(int argc, char* argv[])
{
    // Parse input arguments
    // run commands based on inputs
      
    
    // test a function
        //  set<int> v_set {0,5, 7, 9, 11};
        //  int x = 0;
        //  cout << is_in(x, v_set) << endl;
        //  return 0;
    
    string filename;
    int graph_size=0, num_trials=0;
    int start_node=0;
    float pass1_density, pass2_density;
    Sim_outputs outputs;
    set<int> select_data;

    // parse input arguments
    if (argc == 2) {         // shortest_path_vn filename
        filename = argv[1];  // for testing with input file of graph
    }
    else if (argc == 3) {    // shortest_path_vn size density
        graph_size = atoi(argv[1]);
        pass1_density = atof(argv[2]);  // make 1 random graph
    }
    else if (argc == 5) {               // class assignment simulation inputs
        graph_size = atoi(argv[1]);
        num_trials = atof(argv[2]);
        pass1_density = atof(argv[3]);
        pass2_density = atof(argv[4]);
    }
    else {
        graph_size = 50;                // class assignment defaults
        num_trials = 50;
        pass1_density = 0.2;
        pass2_density = 0.4;
    }

    // running dijkstra based on inputs

    // single run using input file
    if (argc == 2) {  
        Graph graf;  
        graf.load_graph_from_file(filename);
        graf.write_graph("graf.txt");
        auto shortest_paths = Dijkstra(graf, start_node, select_data);
        cout << "avg cost = " << shortest_paths.average_cost() << endl;
        cout << "Found shortest paths: \n";
        cout << shortest_paths << endl;
        
    }
    // generate a random graph and evaluate it
    else if (argc == 3) {
        Graph graf;
        
        // setup optional use of data values (traits) for each node
        const bool use_data=true;
        select_data.insert(1);
        const vector<int> allowed_data {0,1,2};

        graf.make_random_graph(graph_size, pass1_density, use_data, allowed_data, 1.0, 10.0);
        graf.write_graph("graf.txt");
        graf.print_graph();
        auto shortest_paths = Dijkstra (graf, start_node, select_data);
        cout << "avg cost = " << shortest_paths.average_cost() << endl;
        cout << "Found shortest paths: \n";
        cout << shortest_paths << endl;
    }
    // simulating 2 passes per homework assignment
    else {   
        // prepare output file
        string filename = "sim_outputs.txt";
        fstream outfile;
        outfile.open(filename, ios::out);  // open a file to perform write operation using file object
        if (!(outfile.is_open())) {
            cout << "Error opening file: " << filename << " Terminating.\n";
            exit(-1);
        }

        // pass 1
        start_node = 0;
        outputs = run_simulation(start_node, num_trials, graph_size, pass1_density, select_data);
        print_sim_output(outfile, outputs);
        outfile.close();

        // pass 2
        start_node = 0;
        outputs = run_simulation(start_node, num_trials, graph_size, pass2_density, select_data);
        outfile.open(filename, ios::app);
        print_sim_output(outfile, outputs);
        outfile.close();
        
        cout << "Simulation outputs sent to file `sim_outputs.txt`\n";

    }
    
    return 0;
}

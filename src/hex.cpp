// Draw a hexboard and play the game of hex
// Run as ./hex 5 where the single argument is the number of hex's on each
// border of the board For now, the computer plays a naive strategy and a human
// usually wins easily Compile with -std=c++14 or -std=c++11, given features
// I've used. Programmer: Lewis Levin Date: April 2023

#include <algorithm>
#include <cstdio>
#include <deque> // sequence of nodes in a path between start and destination
#include <fstream> // to write graph to file and read graph from file
#include <iostream>
#include <random>
#include <set> // hold nodes for shortest path algorithm
#include <sstream> // to use stringstream to parse inputs from file
#include <stdlib.h> // for atoi()
#include <unordered_map> // container for definition of Graph, costs, previous nodes for Dijkstra
#include <vector>
#include <chrono>      // for performance tuning

using namespace std;


enum class marker { empty = 0, player1 = 1, player2 = 2 };

#define idx(x) static_cast<std::underlying_type_t<decltype(x)>>(x) // shortcut to calc indices for arrays

ostream &operator<<(ostream &os, marker m)
{
    if (m == marker::player1)
        os << " player1 ";
    else if (m == marker::player2)
        os << " player2 ";
    else
        os << " empty ";

    return os;
}


char pause;

// for random shuffling of board moves
random_device rd;
mt19937 randgen(rd());

// send control codes to console to clear the screen
// not guaranteed to work on all OS'es.  Works on MacOs.
void clear_screen() { cout << u8"\033[2J"; }

template <typename T> ostream &operator<<(ostream &os, const set<T> &s)
{
    int count = 0;
    for (const auto &member : s) {
        os << member << " ";
        count++;
        if (count % 10 == 0)
            os << endl;
    }
    return os;
}

template <class T> ostream &operator<<(ostream &os, unordered_map<int, int, T> &um)
{
    int count = 0;
    for (const auto &p : um) {
        os << "    key: " << p.first << " value: " << p.second;
        count++;
        if (count % 4 == 0)
            os << endl;
    }
    return os;
}

ostream &operator<<(ostream &os, deque<int> &dq)
{
    int count = 0;
    for (const auto &p : dq) {
        os << " value: " << p;
        count++;
        if (count % 8 == 0)
            os << endl;
    }
    return os;
}

template <class T> ostream &operator<<(ostream &os, vector<T> &dq)
{
    int count = 0;
    for (const auto &p : dq) {
        os << " value: " << p;
        count++;
        if (count % 8 == 0)
            os << endl;
    }
    return os;
}

// rank and col on the hexboard to address a hexagon
// rank and col are seen by the human player so we use 1-based indexing
// the linear_index conversion function handles this
struct RankCol {
    int rank;
    int col;

    RankCol(int rank = 0, int col = 0) : rank(rank), col(col) {} // initialize to illegal position as sentinel
};

// output a RankCol in an output stream
ostream &operator<<(ostream &os, const RankCol &rc)
{
    os << "Rank: " << rc.rank << " Col: " << rc.col;
    return os;
}

// SOME STRING HELPERS FOR DRAWING THE BOARD
// catenate multiple copies of a string
string string_by_n(string s, int n)
{
    string ret;
    for (int i = 0; i < n; i++) {
        ret += s;
    }
    return ret;
}

// EDGE IS USED WITH THE GRAPH DEFINTION OF A HEXBOARD
// holds an edge for a starting node (aka "edge"): neighbor node, cost to it
// doesn't include source node because that is part of the outer data structure,
// class Graph
struct Edge {
    int to_node; // use linear index to hexboard
    int cost; // default to cost=1 => should NOT change this when creating edges

    Edge(int to_node = 0, int cost = 1) : to_node(to_node), cost(cost) {}
};

// output an Edge in an output stream
ostream &operator<<(ostream &os, const Edge &e)
{
    os << "  to: " << e.to_node << " cost: " << e.cost << endl;
    return os;
}

// output a vector of edges: used in the graph definition for each node
ostream &operator<<(ostream &os, const vector<Edge> &ve)
{
    for (auto const &e : ve) {
        os << e << endl;
    }
    return os;
}

// other non-class functions

// test if value is in set
bool is_in(int val, set<int> v_set) { return v_set.find(val) != v_set.end() ? true : false; }

// test if value is in vector with trivial linear search
bool is_in(int val, vector<int> vec)
{
    auto it = find(vec.begin(), vec.end(), val);
    return it != vec.end();
}

// test if value is in deque with trivial linear search
bool is_in(int val, deque<int> deq)
{
    auto it = find(deq.begin(), deq.end(), val);
    return it != deq.end();
}

// compare to a single value not wrapped in a container
template<typename T>
bool is_in(int val, T one) { return val == one; }

// simple way to time segments of code
// ex:
//      Timing this_timer;
//      this_timer.reset();    // re-uses this_timer and starts it at a new time
//      < bunch-o-code >
//      this_timer.stop();
//      cout << "This code took " << this_timer.ticks() << " seconds\n";  // can be saved in a double, but not in the struct
struct Timing {
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    double duration = 0.0;

    void reset() { start = std::chrono::steady_clock::now(); }

    void stop() { end = std::chrono::steady_clock::now(); }

    double ticks() { return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count(); }
};

Timing game_play_time;  // global to measure cumulative time for assessing the game


// ##########################################################################
// #                            class Graph
// #  graph: data structure holding nodes and their edges
// #  node_data: data structure for holding data value at each node
// #  load_graph_from_file: methods to define graph representation
// #
// ##########################################################################
template <typename T_data>   // use for int, float, enum class
class Graph {
  public:
    Graph<T_data>() = default;
    ~Graph<T_data>() = default;

    // friends
    friend class HexBoard;
    template <typename Z>
    friend class Dijkstra;

    // fields
  private:
    unordered_map<int, vector<Edge>> graph;
    vector<T_data> node_data; // holds Data values of all nodes
    int made_size = 0; // size from loading a graph from file or creating random graph

  public:
    vector<int> all_nodes;

    // methods
  
  public:
    int count_nodes() const { return made_size; }

    void set_node_data(T_data val, int idx) { node_data[idx] = val; }

    T_data get_node_data(int idx) const { return node_data[idx]; }

    // get the neighbors of a node as a vector of edges
    const vector<Edge> get_neighbors(int current_node) const
    {
        return graph.at(current_node); // the value type of graph is
            // vector<Edge> and holds the neighbors
    }

    // get the neighbors that match the select data--the player whose marker is
    // there
    vector<Edge> get_neighbors(int current_node, T_data data_filter) const
    {
        vector<Edge> ve;
        for (auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter) {
                ve.push_back(e);
            }
        }
        return ve;
    }

    // get the neighbors that match the select data and are not in the exclude
    // set, deque or vector
    template <typename Container>
    vector<Edge> get_neighbors(int current_node, T_data data_filter, const Container &exclude_set) const
    {
        vector<Edge> vr;
        for (auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(e.to_node, exclude_set))
                vr.push_back(e);
        }
        return vr; // in c++14 this returns an rvalue reference so the caller
            // moves the returned value to a vector
    }

    // get the neighbor_nodes as a vector of nodes instead of the edges
    vector<int> get_neighbor_nodes(int current_node, T_data data_filter) const
    {
        vector<int> neighbor_nodes;
        for (auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter)
                neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }

    // get the neighbor_nodes that match the filter while excluding a set or vector of nodes
    template <typename Container>
    vector<int> get_neighbor_nodes(int current_node, T_data data_filter, const Container &exclude_set) const
    {
        vector<int> neighbor_nodes;
        for (auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(e.to_node, exclude_set))
                neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }

    // add an edge to the graph
    void add_edge(int x, int y, int cost = 1, bool bidirectional = false)
    {
        if (graph.find(x) != graph.end()) {
            if (graph.find(y) != graph.end()) {
                for (auto edge : graph[x]) { // iterate over edges of x
                    if (y == edge.to_node) {
                        return; // to_node y already exists as an edge of x
                    }
                }
                graph[x].push_back(Edge(y, cost));
                if (bidirectional)
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

    // print graph to an output stream ot
    //    this is the table of nodes and edges in the graph, not a picture of the
    //    game board you can pass an fstream as the output stream cout is the
    //    default value of ot--to screen/console
    void display_graph(ostream &ot = cout, string filename = "")
    {
        int node_id;
        vector<Edge> *edges;

        // print the graph in order
        ot << "\nsize " << graph.size() << "\n";
        for (const int node_id : all_nodes) {
            edges = &(graph.at(node_id));
            ot << "node " << node_id << endl;
            ot << "    data " << node_data[node_id] << "\n";
            for (int j = 0; j < (*edges).size(); j++) {
                ot << "    "
                   << "edge " << edges->at(j).to_node << " " << edges->at(j).cost << endl;
            }
        }
    }

    //
    //     Read a graph file to initialize board graph and positions using the
    //     format of the example: size 4 node 0       // node must be positive
    //     integer; don't need to be consecutive
    //         data 0   // marker at this position
    //         edge 2 3  // edge is to_node, cost. No error checking so to_node must
    //         exist
    //     node 1
    //         data 1
    //         edge 1 4
    //         edge 3 5
    //     node 2
    //         data 1
    //         edge 1 4
    //         edge 3 5
    //     node 3
    //         data 1
    //         edge 1 4
    //         edge 3 5
    //
    //     Note: in this format, edges are assumed to be directional. If you want
    //     bidirectional
    //           (or non-directional that go both ways) then you have to add 2
    //           reciprocal edges.
    //
   void load_graph_from_file(string filename)
    {
        // prepare input file
        ifstream infile;
        infile.open(filename); // open a file to perform read operation using file object
        if (!(infile.is_open())) {
            cout << "Error opening file: " << filename << " Terminating.\n";
            exit(-1);
        }

        // read the file line by line and create graph
        string linestr, leader;
        int node_id, to_node;
        int cost;
        int input_data; // as read from text file
        T_data data;

        // loads data into graph and positions
        while (getline(infile, linestr)) {
            stringstream ss{linestr};
            ss >> leader; // first word or text up to first white space

            if (leader == "size") { // define size constants and reserve memory
                ss >> made_size;

                // reserve storage
                graph.reserve(made_size);
                graph.max_load_factor(0.8);
                node_data.reserve(made_size);

                // initialize node_data
                node_data.insert(node_data.begin(), made_size, static_cast<T_data>(0));
            }
            else if (leader == "node") {
                ss >> node_id;
                graph[node_id] = vector<Edge>(); // create node with empty vector of Edges
                all_nodes.push_back(node_id);
            }
            else if (leader == "edge") {
                ss >> to_node >> cost;
                graph[node_id].push_back(Edge(to_node, cost)); // push Edge to current node_id
            }
            else if (leader == "data") {
                ss >> input_data;
                set_node_data(static_cast<T_data>(data), node_id);
            }
        }
    }
}; // end class Graph



// ##########################################################################
// #             class Dijkstra
// # find minimum cost path, slightly adapted for playing Hex
// # initialized with the graph through which we find the shortest paths
// # inputs from class HexBoard make it possible to find a path of markers
// #    on the board from one player's starting edge to the finish edge
// ##########################################################################
template <typename T_data> // for data held in graph nodes like int, float, enum classes, string
class Dijkstra {
  public:
    Dijkstra(const Graph<T_data> &dijk_graph) : dijk_graph(dijk_graph) {} // initialize the graph to be analyzed.
    ~Dijkstra() = default;

    // fields
  private:
    set<int> path_nodes;
    unordered_map<int, int> path_costs; // dest. node -> cost to reach
    unordered_map<int, deque<int>> path_sequences; // dest. node -> path of nodes this node

  public:
    int start_node;
    const Graph<T_data> &dijk_graph; // need to have a graph to analyze. use reference
        // to avoid copying and we never change it.

    // methods
  public:
    // externally defined methods: effectively the real constructor
    void find_shortest_paths(int, int, bool);
    void find_shortest_paths(int, int, set<int>, bool);

    // print all shortest paths start_node to all other nodes
    friend ostream &operator<<(ostream &os, Dijkstra &dp)
    {
        for (int node : dp.path_nodes) {
            os << "||     Path to " << node << "     ||\n";
            os << "  cost: " << dp.path_costs[node] << "\n";
            os << "  sequence: "
               << "[ ";
            for (auto &seq : dp.path_sequences[node]) {
                os << seq << " ";
            }
            os << "]" << endl;
        }
        return os;
    }

    // methods to navigate resulting path_sequences
    bool path_sequence_exists(int node) { return path_sequences.find(node) != path_sequences.end() ? true : false; }

    // method for caller that doesn't prebuild candidate_nodes
    void find_shortest_paths(int start_here, T_data data_filter, bool verbose = false)
    {
        set<int> candidate_nodes;
        marker node_val = marker::empty;
        // candidates for the shortest paths must match the value in 'data_filter'
        for (auto node : dijk_graph.all_nodes) {
            node_val = dijk_graph.get_node_data(node);
            if (node_val == data_filter) {
                candidate_nodes.insert(node);
            }
        }

        find_shortest_paths(start_here, data_filter, candidate_nodes, false);
    }

    // method for caller that prebuilds appropriate candidate nodes
    void find_shortest_paths(int start_here, T_data data_filter, set<int> candidate_nodes, bool verbose = false)
    {

        int num_nodes = dijk_graph.count_nodes();

        // reserve memory for members and influence number of buckets
        // this improves performance and increases memory consumption, so good for n
        // < 1000
        path_costs.reserve(num_nodes);
        path_costs.max_load_factor(0.8);
        path_sequences.reserve(num_nodes);
        path_sequences.max_load_factor(0.8);

        start_node = start_here;

        // local variables
        const int inf = INT_MAX;
        int neighbor_node = 0;
        int current_node = start_node;
        int prev_node = 0;
        int min = inf;
        int tmp_cost = 0;
        marker node_val;
        deque<int> tmpsequence;
        // set<int> candidate_nodes;   // initialized below with only valid nodes
        vector<Edge> neighbors; // end_node and cost for each neighbor node

        unordered_map<int, int> previous;
        previous.reserve(num_nodes);
        previous.max_load_factor(0.8);

        // candidates for the shortest paths must match the current player in
        // 'data_filter'
        for (auto node : candidate_nodes) {
            node_val = dijk_graph.get_node_data(node);
            if (node_val == data_filter) {
                // candidate_nodes.insert(node);
                path_costs[node] = inf; // initialize costs to inf for Dijkstra alg.
            }
        }

        path_costs[start_node] = 0;
        previous[start_node] = -1;

        // algorithm loop
        while (!(candidate_nodes.empty())) {
            // current_node begins as start_node

            if (verbose)
                cout << "\ncurrent_node at top of loop " << current_node << endl;

            if (dijk_graph.get_node_data(current_node) != data_filter)
                break;

            neighbors = dijk_graph.get_neighbors(current_node, data_filter,
                                                 path_nodes); //, path_nodes);  // vector<Edge>
            for (auto &neighbor : neighbors) { // neighbor is an Edge
                neighbor_node = neighbor.to_node;
                tmp_cost = path_costs[current_node] + neighbor.cost; // update path_costs for neighbors of current_node
                if (tmp_cost < path_costs[neighbor_node]) {
                    path_costs[neighbor_node] = tmp_cost;
                    previous[neighbor_node] = current_node;
                }
            }

            if (current_node == start_node && neighbors.empty())
                break; // start_node is detached from all others

            if (verbose) {
                //  for debugging shortest path algorithm
                cout << "**** STATUS ****\n";
                cout << "  current_node " << current_node << endl;
                cout << "  neighbors\n";
                cout << neighbors << endl;
                cout << "  path_nodes\n";
                cout << "    " << path_nodes << endl;
                cout << "  candidate_nodes\n";
                cout << "    " << candidate_nodes << endl;
                cout << "  previous\n";
                cout << previous << endl;
                cout << "  path cost\n";
                cout << path_costs << endl;
            }

            path_nodes.insert(current_node);
            candidate_nodes.erase(current_node); // we won't look at this node again

            // pick next current_node based on minimum distance, which will be a
            // neighbor whose cost has been updated from infinity to the network
            // edge cost
            min = inf;
            for (const int tmp_node : candidate_nodes) { // always O(n)
                if (path_costs[tmp_node] < min) {
                    min = path_costs[tmp_node];
                    current_node = tmp_node;
                }
            }

            if (min == inf) { // none of the remaining nodes are reachable
                break;
            }

            if (verbose)
                cout << "  current node at bottom: " << current_node << endl;
        }

        // build sequences by walking backwards from each dest. node to start_node
        for (const auto walk_node : path_nodes) {
            prev_node = walk_node;
            while (prev_node != start_node) {
                tmpsequence.push_front(prev_node); // it's a deque
                prev_node = previous[prev_node];
            };
            tmpsequence.push_front(prev_node);
            path_sequences[walk_node] = tmpsequence;

            tmpsequence.clear(); // IMPORTANT: clear before reusing to build a new
                // sequence!
        }

        if (verbose)
            cout << *this << endl; // prints the nodes and shortest path_sequences
                // to each node
    }
};
// end of class Dijkstra

// ##########################################################################
// #             end of class Dijkstra methods
// ##########################################################################


// ##########################################################################
// #                            class HexBoard
// #  creates a Graph object as a member of HexBoard using one either
// #      method load_board_from_file or make_board
// #  methods for defining and drawing the pictorial representation of the board
// #  methods for a person to play against this program
// #
// ##########################################################################
// template<typename T_data>   // use for the type of data stored at a node: typically int, float, enum classes
class HexBoard {
  public:
    HexBoard() = default;
    ~HexBoard() = default;

    // classes used to define graph and find longest paths on the board
    template <typename K>
    friend class Graph;
    template <typename L>
    friend class Dijkstra;
  

  public:
    // a member of HexBoard that is a Graph object, using "composition" instead
    // of inheritance the actual graph is created by either method: make_board
    // or load_board_from_file
    Graph<marker> hex_graph;
    int edge_len = 0;
    int max_rank = 0; // and equals max_col -> so, only need one
    int max_idx = 0; // maximum linear index
    int move_count = 0; // number of moves played during the game
    vector<int> &all_nodes = hex_graph.all_nodes; // maintains sorted list of nodes by linear index
    vector<int> rand_nodes; // use to shuffle nodes for monte carlo simulation
        // of game moves

  private:
    vector<marker> &positions = hex_graph.node_data; // a reference to the graph's node_data

    vector<vector<int>> start_border; // holds indices at the top and left edges of the board
    vector<vector<int>> finish_border; // holds indices at the bottom and right edges of the board

    vector<vector<RankCol>> move_seq; // history of moves: use ONLY indices 1 and 2 for outer vector

    // methods
  private:
    // METHODS FOR DRAWING THE BOARD
    // return hexboard marker based on value
    //     and add the spacer lines ___ needed to draw the board
    string symdash(marker val, bool last = false)
    {
        string symunit;
        string dot = "."; // for markers encoded as 0 == empty
        string x = "X"; // for markers encoded as 1
        string o = "O"; // for markers encoded as 2
        string spacer = "___"; // part of drawing the board as ascii characters

        if (last)
            spacer = "";

        if (val == marker::empty)
            symunit = dot + spacer;
        else if (val == marker::player1)
            symunit = x + spacer;
        else if (val == marker::player2)
            symunit = o + spacer;
        else {
            cout << "Error: invalid hexboard value: " << val << endl;
            exit(-1);
        }

        return symunit;
    }

    // how many spaces to indent each line of the hexboard?
    string lead_space(int rank) { return string_by_n(" ", rank * 2); }

    // some string constants used to draw the board
    const string connector = R"( \ /)";
    const string last_connector = R"( \)";

    // create sets containing start and finish borders for both sides
    void board_regions() // tested OK
    {
        int rank;
        int col;

        // yes--we could this all in one loop but, this is much more obvious

        // initialize the inner vectors as empty. NOTE: the zero index for the outer
        // vector should NEVER be used
        for (int i = 0; i < 3; i++) {
            start_border.push_back(vector<int>{});
            finish_border.push_back(vector<int>{});
        }

        for (int rank = 1, col = 1; col < edge_len + 1; col++) {
            start_border[idx(marker::player1)].push_back(linear_index(rank, col));
        }
        for (int rank = edge_len, col = 1; col < edge_len + 1; col++) {
            finish_border[idx(marker::player1)].push_back(linear_index(rank, col));
        }

        for (int rank = 1, col = 1; rank < edge_len + 1; rank++) {
            start_border[idx(marker::player2)].push_back(linear_index(rank, col));
        }

        for (int rank = 1, col = edge_len; rank < edge_len + 1; rank++) {
            finish_border[idx(marker::player2)].push_back(linear_index(rank, col));
        }
    }

  public:
    // externally defined methods to create a valid Hex game board and graph
    // representation initialize members create graph structure of board by
    // adding all edges

    void make_board(int border_len) // initialize board positions
    {
        // initialize all members
        edge_len = border_len;
        max_rank = edge_len - 1;
        max_idx = edge_len * edge_len; // same as size

        // REMINDER!!!: rank and col indices are treated as 1-based!

        // reserve storage
        positions.reserve(max_idx);

        // define the board regions and move sequences
        board_regions();
        initialize_move_seq();

        // initialize positions: and also node_data because positions is a reference
        // to it
        positions.insert(positions.begin(), max_idx, marker::empty);

        // reserve memory and initialize int for Graph member hex_graph
        hex_graph.graph.reserve(max_idx);
        hex_graph.graph.max_load_factor(0.8);
        hex_graph.made_size = max_idx;

        // add nodes:  the required hexagonal "tiles" on the board
        // initial values:  all tiles are empty = 0
        for (int i = 0; i < max_idx; i++) {
            all_nodes.push_back(i); // set of nodes used to find paths for both HexBoard and Graph
            hex_graph.graph[i] = vector<Edge>(); // create empty edge list for each tile (aka, node)
            rand_nodes.push_back(i); // vector of nodes
        }

        // add graph edges for adjacent hexes based on the layout of a Hex game
        // board 4 corners                                tested OK upper left
        hex_graph.add_edge(linear_index(1, 1), linear_index(2, 1));
        hex_graph.add_edge(linear_index(1, 1), linear_index(1, 2));
        // lower right
        hex_graph.add_edge(linear_index(edge_len, edge_len), linear_index(edge_len, max_rank));
        hex_graph.add_edge(linear_index(edge_len, edge_len), linear_index(max_rank, edge_len));
        // upper right
        hex_graph.add_edge(linear_index(1, edge_len), linear_index(1, max_rank));
        hex_graph.add_edge(linear_index(1, edge_len), linear_index(2, edge_len));
        hex_graph.add_edge(linear_index(1, edge_len), linear_index(2, max_rank));
        // lower left
        hex_graph.add_edge(linear_index(edge_len, 1), linear_index(max_rank, 1));
        hex_graph.add_edge(linear_index(edge_len, 1), linear_index(edge_len, 2));
        hex_graph.add_edge(linear_index(edge_len, 1), linear_index(max_rank, 2));

        // 4 borders (excluding corners)  4 edges per node.
        // north-south edges: constant rank, vary col
        for (int c = 2; c < edge_len; c++) {
            int r = 1;
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c + 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c));

            r = edge_len;
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c + 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c));
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c + 1));
        }
        // east-west edges: constant col, vary rank
        for (int r = 2; r < edge_len; r++) {
            int c = 1;
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c));
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c + 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c + 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c));

            c = edge_len;
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c));
        }

        // interior tiles: 6 edges per hex
        for (int r = 2; r < edge_len; r++) {
            for (int c = 2; c < edge_len; c++) {
                hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c + 1));
                hex_graph.add_edge(linear_index(r, c), linear_index(r, c + 1));
                hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c));
                hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c - 1));
                hex_graph.add_edge(linear_index(r, c), linear_index(r, c - 1));
                hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c));
            }
        }

        // hex_graph.display_graph();
        // cin >> pause;
    } // end of make_board

    void load_board_from_file(string filename)
    {
        hex_graph.load_graph_from_file(filename);

        // initialize HexBoard class members
        max_idx = hex_graph.count_nodes();
        edge_len = sqrt(max_idx);
        max_rank = edge_len - 1;

        if (edge_len * edge_len != max_idx) {
            cout << "Error: incorrect size for hexboard. Got size = " << max_idx << endl;
            cout << "Size must have an integer square root for number of tiles in "
                    "edge "
                    "of board.\n";
            exit(-1);
        }

        // define the board regions and move sequences
        board_regions();
        initialize_move_seq();
    }

    // methods for playing game externally defined



    void initialize_move_seq()
    {

        // for move_seq for players 1 and 2
        for (int i = 0; i < 3; i++) {
            move_seq.push_back(vector<RankCol>{});
            if (i > 0)
                move_seq[i].reserve(max_idx / 2 + 1);
        }
    }

    // print the ascii board on screen
    void display_board()
    {
        {
            bool last; // last board value in the rank: true or false?

            // format two lines for each rank (except the last)
            for (int rank = 1; rank < edge_len + 1; rank++) {
                cout << lead_space(rank);
                for (int col = 1; col < edge_len + 1; col++) {
                    last = col < edge_len ? false : true;
                    cout << symdash(get_hex_marker(rank, col), last); // add each column value
                }

                cout << endl; // line break for rank

                // connector lines to show edges between board positions
                if (rank != edge_len) {
                    cout << lead_space(rank); // leading spaces for connector line
                    cout << string_by_n(connector, max_rank) << last_connector << endl;
                }
                else {
                    cout << "\n\n"; // last rank: no connector slashes
                }
            }
        }
    }

    // GAME PLAY METHODS
    void simulate_hexboard_positions() // utility function creates random board
        // positions
    {
        marker val;
        for (int i = 0; i < hex_graph.node_data.size(); i++) {
            while (val == marker::empty) {
                val = static_cast<marker>(rand() % 3);
            }
            hex_graph.node_data[i] = val;
            val = marker::empty;
        }
    }
    // ##########################################################################
    // #             Class HexBoard game playing methods
    // ##########################################################################

    // the program makes a random move
    RankCol random_move()
    {
        RankCol rc;
        int maybe; // candidate node to place a marker on

        shuffle(rand_nodes.begin(), rand_nodes.end(), randgen);

        for (int i = 0; i < max_idx; i++) {
            maybe = rand_nodes[i];
            if (is_empty(maybe)) {
                rc = rank_col_index(maybe);
                break;
            }
        }
        if (rc.rank == 0 && rc.col == 0) { // never found an empty position->no possible move
            rc.rank = -1;
            rc.col = -1; // no move
        }

        return rc;
    }

    // use for testing only
    RankCol prompt_for_computer_move(marker side)
    {

        RankCol rc;
        int rank;
        int col;
        int val;
        bool valid_move = false;
        char pause;

        while (!valid_move) {
            cout << "Enter a move for the computer.\n";
            cout << "Enter the rank (the row, using Chess terminology)... ";

            cin >> rank;

            cout << "Enter the column... ";

            cin >> col;

            rc.rank = rank;
            rc.col = col;
            valid_move = is_valid_move(rc);
        }
        set_hex_marker(side, rc);

        return rc;
    }

    // the program makes a naive move to extend its longest path
    RankCol naive_move()
    {
        RankCol rc;
        RankCol prev_move;
        int prev_move_linear;
        vector<int> neighbor_nodes;
        char pause;

        if (move_seq[idx(marker::player2)].empty()) {
            shuffle(start_border[static_cast<int>(marker::player2)].begin(),
                    start_border[static_cast<int>(marker::player2)].end(), randgen);
            for (int maybe : start_border[static_cast<int>(marker::player2)]) {
                if (is_empty(maybe)) {
                    rc = rank_col_index(maybe);
                    return rc;
                }
            }
        }
        else {
            prev_move = move_seq[idx(marker::player2)].back();
            prev_move_linear = linear_index(prev_move);

            neighbor_nodes = hex_graph.get_neighbor_nodes(prev_move_linear, marker::empty);

            if (neighbor_nodes.empty())
                return random_move();

            shuffle(neighbor_nodes.begin(), neighbor_nodes.end(), randgen);
            shuffle(neighbor_nodes.begin(), neighbor_nodes.end(), randgen);

            for (int node : neighbor_nodes) {
                rc = rank_col_index(node);
                if (rc.col > prev_move.col)
                    return rc;
            }
            rc = rank_col_index(neighbor_nodes.back());
        }

        return rc;
    }

    RankCol computer_move(marker side)
    {
        RankCol rc;
        //  rc = random_move();   // this is WORSE than naive...

        rc = naive_move();

        return rc;
    }

    RankCol prompt_for_person_move(marker side)
    {
        RankCol rc;
        int rank;
        int col;
        int val;
        bool valid_move = false;

        while (!valid_move) {
            cout << "You go first playing X markers. The computer goes second "
                    "playing "
                    "O markers.\n";
            cout << "Enter a move in an empty position that contains '.'" << endl;
            cout << "(A Note for c++ programmers: end-users use 1-indexing, so "
                    "that's "
                    "what we use...)\n";
            cout << "Enter -1 to quit..." << endl;
            cout << "Enter the rank (the row, using Chess terminology)... ";

            cin >> rank;

            if (rank == -1) {
                rc.rank = -1;
                rc.col = -1;
                return rc;
            }

            if (rank == -5) { // hidden command to write the current board positions
                // to a file
                // prepare output file
                string filename = "Board Graph.txt";
                fstream outfile;
                outfile.open(filename,
                             ios::out); // open a file to perform write operation
                // using file object
                if (!(outfile.is_open())) {
                    cout << "Error opening file: " << filename << " Terminating.\n";
                    exit(-1);
                }
                hex_graph.display_graph(outfile);
            }

            cout << "Enter the column... ";

            cin >> col;

            if (col == -1) {
                rc.rank = -1;
                rc.col = -1;
                return rc;
            }

            rc.rank = rank;
            rc.col = col;
            valid_move = is_valid_move(rc);
        }

        return rc;
    }

    bool is_valid_move(RankCol rc)
    {
        int rank = rc.rank;
        int col = rc.col;

        string bad_position = "Your move used an invalid rank or column.\n\n";
        string not_empty = "Your move didn't choose an empty position.\n\n";
        string msg = "";

        bool valid_move = true;

        if (rank > edge_len || rank < 1) {
            valid_move = false;
            msg = bad_position;
        }
        else if (col > edge_len || col < 1) {
            valid_move = false;
            msg = bad_position;
        }
        else if (get_hex_marker(rc) != marker::empty) {
            valid_move = false;
            msg = not_empty;
        }

        cout << msg;

        return valid_move;
    }

    // follows path through the board for one side
    marker follow_paths(vector<int> linear_moves, marker side, bool full_board = false)
    {
        marker winner = marker::empty;
        int current_node = 0;
        int this_neighbor = 0;
        char pause;
        deque<vector<int>> working; // MUST BE A DEQUE! hold candidate sequences across the board
        vector<int> neighbors;
        neighbors.reserve(6);
        vector<int> captured; // nodes that have been put into a candidate path
        captured.reserve(max_idx / 2 + 1);

        //  cout << "linear moves " << linear_moves << endl;

        // test for finish borders, though start border would also work: assumption fewer markers at the finish
        if (side == marker::player1) {
            for (auto move : linear_moves) {
                if (is_in_finish(move, side)) // (i < edge_len)
                {
                    working.emplace_front(vector<int>{move});
                    working.front().reserve(4 * edge_len);
                    captured.push_back(move);
                }
            }

            // cout << "size of working " << working.size() << endl;
            // cout << working.front() << endl;
        }
        else if (side == marker::player2) {
            for (auto move : linear_moves) {
                if (is_in_finish(move, side)) {
                    working.emplace_front(vector<int>{move});
                    captured.push_back(move);
                }
            }
        }
        else {
            exit(-1);
        }

        // cout << "size of working " << working.size() << endl;
        // cin >> pause;

        // shortcut: if full_board and no paths start in starting border then the other side has one
        if (full_board && working.empty())
            return side == marker::player1 ? marker::player2 : marker::player1;

        // extend sequences that begin in the start border to see if any reach the finish border
        // grab a sequence  (sequences have no branches:  a branch defines a new sequence)
        // get the neighbors of the last node
        // if more than one neighbors, we add another working sequence for neighbors 2 through n
        // if no more neighbors we look at the end:
        // if we get to the finish border, we have a winner
        // otherwise, we can discard the current sequence and grab another
        // if we exhaust the sequences without finding a winner, the other side wins

        //  cout << "got to top of loop";
        //  cin >> pause;

        while (!working.empty()) {
            auto &this_seq = working.back(); // grab a sequence

            // cout << "beginning of seq " << this_seq << endl;

            while (true) // extend, branch, reject, find winner for this sequence
            {
                current_node = this_seq.back();

                neighbors = hex_graph.get_neighbor_nodes(current_node, side, captured);

                //   cout << "neighbors are: " << neighbors << endl;
                //   cout << "pause... (any char and enter to continue)";
                //   cin >> pause;

                if (neighbors.empty()) {

                    //   cout << "Got to neighbors empty\n" ;
                    //   cout << this_seq << endl;
                    //   cin >> pause;

                    if (is_in_start(current_node, side)) { // if node in start border we have a winner
                        // cout << "Side " << side << " won at finish border with no neighbors\n";
                        return winner = side;
                    }
                    else {
                        if (!working.empty())
                            working.pop_back(); // discard the current sequence
                        break; // break loop for this sequence: get another sequence
                    }
                }
                else if (neighbors.size() == 1) {
                    // add new node to this_seq and continue to extend

                    //   cout << "current node " << current_node << " number of neighbors " << neighbors.size() << endl;
                    //   cout << neighbors << endl;
                    //   cin >> pause;

                    this_seq.push_back(neighbors.front());
                    captured.push_back(neighbors.front());
                }
                else {
                    for (int i = 0; i < neighbors.size(); i++) {
                        this_neighbor = neighbors[i];

                        //   cout << "got to branching path ";
                        //   cin >> pause;

                        if (i == 0) { // extend the current seq
                            this_seq.push_back(this_neighbor);
                            captured.push_back(this_neighbor);
                        }
                        else { // make a new seq
                            working.emplace_back(this_seq.begin(), this_seq.end() - 1); // copy up to next-to-last node
                            // working.back().reserve(4*edge_len);
                            working.back().push_back(
                                this_neighbor); // add the current neighbor going in a new direction
                            captured.push_back(this_neighbor);
                        }

                        //   cout << "size of working " << working.size() << endl;
                        // cin >> pause;
                    }
                }
            } // while(true)
        } // while (!working.empty())
        return marker::empty; // we did not find a winner
    }

    marker who_won_dijkstra()
    {
        marker side = marker::empty;
        marker winner = marker::empty;
        int finish_hex;
        int start_hex;
        set<int> candidates;

        // use Dijkstra shortest path algorithm to find winner
        // test for side one victory
        // for each hex in the finish_border for side 1, find a path that ends in
        // start_border for player 1

        // for (int col = 1, fin = linear_index(1, col); col < edge_len+1; col++)
        for (int finish_hex : finish_border[idx(marker::player1)]) {
            if (get_hex_marker(finish_hex) != marker::player1)
                continue;
            Dijkstra<marker> paths(hex_graph);

            copy_move_seq(move_seq[idx(marker::player1)], candidates);
            // must be a copy because we remove items from candidates AND we need to keep move_seq

            // find a path that starts from the finish border
            paths.find_shortest_paths(finish_hex, marker::player1, candidates);
            for (int start_hex : start_border[static_cast<int>(marker::player1)]) {
                if (paths.path_sequence_exists(start_hex)) { // to the starting border
                    winner = marker::player1;
                    return winner;
                }
            }
        }

        // test for side two victory
        // for each hex in finish border of side 2, find a path that ends in start border of side 2
        for (int finish_hex : finish_border[idx(marker::player2)]) {
            if (get_hex_marker(finish_hex) != marker::player2)
                continue;
            Dijkstra<marker> paths(hex_graph);

            copy_move_seq(move_seq[idx(marker::player2)], candidates);

            paths.find_shortest_paths(finish_hex, marker::player2, candidates);
            for (int start_hex : start_border[idx(marker::player2)]) {
                if (paths.path_sequence_exists(start_hex)) {
                    winner = marker::player2;
                    return winner;
                }
            }
        }

        return winner; // will always be 0 or marker::empty
    }

    marker who_won()
    {

        game_play_time.reset();

        marker winner = marker::empty;
        vector<int> candidates;
        vector<marker> sides{marker::player1, marker::player2};

        for (auto s : sides) {
            copy_move_seq(move_seq[idx(s)], candidates);
            winner = follow_paths(candidates, s);
            if (winner == s) {
                break;
            }
        }

        game_play_time.stop();
        game_play_time.duration += game_play_time.ticks();

        return winner; // will always be 0
    }

    void play_game(bool simulate = false)
    {
        RankCol rc; // person's move
        RankCol computer_rc; // computer's move
        char pause;
        bool valid_move;
        bool end_game = false;
        marker winning_side;

        if (simulate) {
            simulate_hexboard_positions();
        }
        else {
            while (true) // move loop
            {
                clear_screen();
                cout << "\n\n";
                display_board();

                rc = prompt_for_person_move(marker::player1);
                if (rc.rank == -1) {
                    cout << "Game over! Come back again...\n";
                    break;
                }

                set_hex_marker(marker::player1, rc);
                move_seq[idx(marker::player1)].push_back(rc);

                display_board();
                computer_rc = computer_move(marker::player2);
                set_hex_marker(marker::player2, computer_rc);
                move_seq[idx(marker::player2)].push_back(computer_rc);

                move_count++;

                clear_screen();

                // test for a winner
                if (move_count >= edge_len) {

                    //              cout << "Got to testing for winner... (enter any char and press enter)\n";
                    //              cin >> pause;

                    winning_side = who_won(); // result is marker::empty, marker::player1, or marker::player2
                    if (idx(winning_side)) {
                        cout << "We have a winner. "
                             << (winning_side == marker::player1 ? "You won. Congratulations!"
                                                                 : " The computer beat you )-:")
                             << "\nGame over. Come back and play again\n\n";
                        display_board();
                        break;
                    }
                }

                cout << "Your move at " << rc << " is valid.\n";
                cout << "The computer moved at " << computer_rc << ". Move count = " << move_count << endl;
            }
        }
    }

    // copy RankCol positions to a set of ints.   caller provides destination
    // container:  set or vector.
    void copy_move_seq(vector<RankCol> moves, set<int> &cands)
    {
        int node;
        cands.clear();
        for (auto m : moves) {
            node = linear_index(m);
            cands.insert(node);
        }
    }

    void copy_move_seq(vector<RankCol> moves, vector<int> &cands)
    {
        int node;

        cands.clear();
        for (auto m : moves) {
            node = linear_index(m);
            cands.push_back(node);
        }
    }

    // game play methods use rank and col for board positions
    // rank and col indices are 1-based for end users playing the game
    void set_hex_marker(marker val, RankCol rc) { positions[linear_index(rc)] = val; }

    void set_hex_marker(marker val, int rank, int col) { positions[linear_index(rank, col)] = val; }

    void set_hex_marker(marker val, int linear) { positions[linear] = val; }

    marker get_hex_marker(RankCol rc) { return positions[linear_index(rc)]; }

    marker get_hex_marker(int rank, int col) { return positions[linear_index(rank, col)]; }

    marker get_hex_marker(int linear) { return positions[linear]; }

    // convert rank, col position to a linear index to an array or map
    // graph and minimum cost path use linear indices
    // linear indexes are 0-based to access c++ data structures
    int linear_index(RankCol rc)
    {
        int rank;
        int col;

        rank = rc.rank - 1; // convert from input 1-based indexing to zero-based indexing
        col = rc.col - 1;
        if (rank < edge_len && col < edge_len) {
            return (rank * edge_len) + col;
        }
        else {
            cout << "Error: rank or col >= edge length\n";
            return -1;
        }
    }

    // convert rank, col position to a linear index to an array or map
    int linear_index(int rank, int col)
    {
        rank -= 1; // convert from input 1-based indexing to zero-based indexing
        col -= 1;
        if (rank < edge_len && col < edge_len) {
            return (rank * edge_len) + col;
        }
        else {
            cout << "Error: rank or col >= edge length\n";
            return -1;
        }
    }

    // convert linear_index to RankCol index
    RankCol rank_col_index(int linear)
    {
        if (linear < max_idx) {
            return RankCol((linear / edge_len) + 1, (linear % edge_len) + 1);
        }
        else {
            cout << "Error: rank or col >= edge length\n";
            return RankCol(-1, -1);
        }
    }

    bool is_in_start(int idx, marker side)
    {

        if (side == marker::player1) {
            return idx < edge_len;
        }
        else if (side == marker::player2) {
            return idx % edge_len == 0;
        }
        else
            exit(-1);
    }

    bool is_in_finish(int idx, marker side)
    {

        if (side == marker::player1) {
            return idx > max_idx - edge_len - 1;
        }
        else if (side == marker::player2) {
            return idx % edge_len == edge_len - 1;
        }
        else {
            exit(-1);
        }
    }

    inline bool is_empty(RankCol rc) { return is_empty(linear_index(rc)); }

    inline bool is_empty(int linear) { return positions[linear] == marker::empty; }

    inline int count_nodes() const { return positions.size(); }

    inline marker get_node_data(int node) const { return positions[node]; }
};
// ######################################################
// end class hexboard
// ######################################################




int main(int argc, char *argv[])
{
    srand(time(0));

    int size = 5;

    if (argc == 2)
        size = atoi(argv[1]);

    if (size > 11) {
        cout << "Your board size might be a little large: try between 5 and "
                "11.\n";
        cout << "Let's go ahead and play a tiny game with a 5 x 5 board.\n";
        size = 5;
    }
    else if (size < 5) {
        cout << "Size should be from 5 through 11.\n";
        cout << "Let's go ahead and play a tiny game with a 5 x 5 board.\n";
        size = 5;
    }

    Timing this_timer;

    this_timer.reset();

    HexBoard hb;
    hb.make_board(size);

    this_timer.stop();
    cout << "Setup took " << this_timer.ticks() << " seconds.\n";
    cout << "Paused... (enter any key and press enter to continue) ";
    cin >> pause;

    hb.play_game();

    cout << "Assessing who won took " << game_play_time.duration << " seconds.\n";


    return 0;
}

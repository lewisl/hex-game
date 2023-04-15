// Draw a hexboard and play the game of hex
// Run as ./hex 5 where the single argument is the number of hex's on each border of the board
// For now, the computer plays a naive strategy and a human usually wins easily
// Compile with -std=c++14 or -std=c++11, given features I've used.
// Programmer: Lewis Levin
// Date: April 2023

#include <iostream>
#include <stdlib.h>         // for atoi()
#include <random>
#include <vector>
#include <deque>            // sequence of nodes in a path between start and destination
#include <unordered_map>    // container for definition of Graph, costs, previous nodes for Dijkstra
#include <set>              // hold nodes for shortest path algorithm
#include <fstream>          // to write graph to file and read graph from file
#include <sstream>          // to use stringstream to parse inputs from file

using namespace std;

const int PLAYER1_X = 1;
const int PLAYER2_O = 2;
const int EMPTY_HEX = 0;

// for random shuffling of board moves
random_device rd;
mt19937 randgen(rd());

// send control codes to console to clear the screen
// not guaranteed to work on all OS'es.  Works on MacOs.
void clear_screen()
{
    cout << u8"\033[2J";
}

template <typename T>
ostream& operator<< (ostream& os, const set<T> & s)
{
    int count = 0;
    for (const auto &member : s) {
        os << member << " ";
        count++;
        if (count % 10 == 0) os << endl;
    }
    return os;
}

template <class T>
ostream& operator<< (ostream& os, unordered_map<int, int, T> & um)
{
    int count = 0;
    for (const auto& p : um) {
        os << "    key: " << p.first << " value: " << p.second;
        count++;
        if (count % 4 == 0) os << endl;
    }
    return os;
}

ostream& operator<< (ostream& os, deque<int> & dq)
{
    int count = 0;
    for (const auto& p : dq) {
        os << " value: " << p;
        count++;
        if (count % 8 == 0) os << endl;
    }
    return os;
}

template<class T>
ostream& operator<< (ostream& os, vector<T> & dq)
{
    int count = 0;
    for (const auto& p : dq) {
        os << " value: " << p;
        count++;
        if (count % 8 == 0) os << endl;
    }
    return os;
}

// rank and col on the hexboard to address a hexagon
    // rank and col are seen by the human player so we use 1-based indexing
    // the linear_index conversion function handles this
struct RankCol
{
    int rank;
    int col;

    RankCol(int rank=0, int col=0) : rank(rank), col(col) {}  // initialize to illegal position as sentinel
};

// output a RankCol in an output stream
ostream& operator<< (ostream& os, const RankCol& rc)
{
    os << "Rank: " << rc.rank << " Col: " << rc.col;
    return os;
}

// SOME STRING HELPERS FOR DRAWING THE BOARD
// catenate multiple copies of a string
string string_by_n(string s, int n)
{
    string ret;
    for (int i=0; i<n; i++) {
        ret += s;
    }
    return ret;
}



// EDGE IS USED WITH THE GRAPH DEFINTION OF A HEXBOARD
// holds an edge for a starting node (aka "edge"): neighbor node, cost to it
// doesn't include source node because that is part of the outer data structure, class Graph
struct Edge 
{  
    int to_node;   // use linear index to hexboard    
    int cost;      // default to cost=1 => should NOT change this when creating edges
    
    Edge(int to_node=0, int cost=1) : to_node(to_node), cost(cost) {}
};

// output an Edge in an output stream
ostream& operator<< (ostream& os, const Edge& e)
{
    os << "  to: " << e.to_node << " cost: " << e.cost << endl;
    return os;
}

// output a vector of edges: used in the graph definition for each node
ostream& operator<< (ostream& os, const  vector<Edge> & ve)
{
    for (auto const & e : ve) {
        os << e << endl;
    }
    return os;
}

// other non-class functions

// test if value is in set
bool is_in(int val, set<int> v_set)
{
    return v_set.find(val) != v_set.end() ? true : false;
}

// test if value is in vector with trivial linear search
bool is_in(int val, vector<int> vec)
{
    bool ret = false;
    for (int v : vec)
        {
            if (val == v) {
                ret = true;
                break;
            }
        }
    return ret;
}


// test if value is in deque with trivial linear search
bool is_in(int val, deque<int> deq)
{
    bool ret = false;
    for (int d : deq)
        {
            if (val == d) {
                ret = true;
                break;
            }
        }
    return ret;
}


// hash function for unorderered_map<int, int>
// sometimes marginally faster for low-valued ints in small tables

unsigned int jenkins_hash(unsigned int key) {
  key = (key + 0x7ed55d16) + (key << 12);
  key = (key ^ 0xc761c23c) ^ (key >> 19);
  key = (key + 0x165667b1) + (key << 5);
  key = (key + 0xd3a2646c) ^ (key << 9);
  key = (key + 0xfd7046c5) + (key << 3);
  key = (key ^ 0xb55a4f09) ^ (key >> 16);
  return key;
}

struct IntHash {
  std::size_t operator()(int key) const {
    return static_cast<std::size_t>(
        jenkins_hash(static_cast<unsigned int>(key))
                                    );
  }
};



class HexBoard;   // forward declaration for need for class Graph


// ##########################################################################
// #                            class Graph
// #  graph: data structure holding nodes and their edges
// #  node_data: data structure for holding data value at each node
// #  load_graph_from_file: methods to define graph representation
// #
// ##########################################################################
class Graph
{
public:

    Graph() = default;
    ~Graph() = default;

// friends
    friend class HexBoard;
    friend class Dijkstra;
    
// fields
private:
    unordered_map<int, vector<Edge>, IntHash> graph;  
    vector<int> node_data;                  // holds Data values of all nodes
    int made_size=0;                        // size from loading a graph from file or creating random graph
    
public:
    vector<int> all_nodes;
    
// methods
private:
    
public:
    // externally defined public methods
    void display_graph(ostream&, string);  
    void load_graph_from_file(string);
    void add_edge(int, int, int, bool);
    
    int count_nodes() const
    {
        return made_size;
    }
    
    void set_node_data(int val, int idx)
    {
        node_data[idx] = val;
    }
    
    int get_node_data(int idx) const
    {
        return node_data[idx];
    }
    
    // get the neighbors of a node as a vector of edges
    const vector<Edge>  get_neighbors(int current_node) const
    {
        return graph.at(current_node);  //the value type of graph is vector<Edge> and holds the neighbors
    }
        
    // get the neighbor_nodes as a vector of nodes instead of the edges
    vector<int> get_neighbor_nodes(int currrent_node, int data_filter) const
    {
        vector<Edge> ve;
        vector<int> neighbor_nodes;
        
        ve = get_neighbors(currrent_node, data_filter);
        for (auto e : ve) {
            neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }
    
    // get the neighbors that match the select data--the player whose marker is there
    vector<Edge> get_neighbors(int current_node, int data_filter) const
    {
        vector<Edge>  ve;
        for (auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter) {
                ve.push_back(e);
            }
        }
        return ve;
    }
    
    // get the neighbors that match the select data and are not in the exclude set
    vector<Edge> get_neighbors(int current_node, int data_filter, set<int> exclude_set) const 
    {
        vector<Edge> ve;
        for (auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(node_data[e.to_node], exclude_set)) {
                ve.push_back(e);
            }
        }
        return ve;  // in c++14 this returns an rvalue reference so the caller moves the returned value to a vector
    }  
};  // end class Graph

// ##########################################################################
//     externally defined class Graph methods
// ##########################################################################

// add an edge to the graph
void Graph::add_edge(int x, int y, int cost=1, bool bidirectional=false)
{
    if (graph.find(x) != graph.end()) {
        if (graph.find(y) != graph.end()) {
            for (auto edge : graph[x]) {    // iterate over edges of x
                if (y == edge.to_node) {
                    return;                 // to_node y already exists as an edge of x
                }
            }
            graph[x].push_back(Edge(y, cost));  
            if (bidirectional) graph[y].push_back(Edge(x, cost));
            
        }
        else {
            cout << "node " << y << " not found in graph.";
        }
    }
    else {
        cout << "node " << x << " not found in graph.";
    }
}

// 
//     Read a graph file to initialize board graph and positions using the format of the example:
//     size 4
//     node 0       // node must be positive integer; don't need to be consecutive
//         data 0   // marker at this position
//         edge 2 3  // edge is to_node, cost. No error checking so to_node must exist
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
//     Note: in this format, edges are assumed to be directional. If you want bidirectional 
//           (or non-directional that go both ways) then you have to add 2 reciprocal edges.
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
    int data;
    
    // loads data into graph and positions
    while (getline(infile, linestr))
        {
            stringstream ss {linestr};
            ss >> leader;    // first word or text up to first white space
            
            if (leader == "size") {  // define size constants and reserve memory
                ss >> made_size;
                
                // reserve storage
                graph.reserve(made_size);
                node_data.reserve(made_size);
                
                // initialize node_data
                node_data.insert(node_data.begin(), made_size, 0);
            }
            else if (leader == "node") {
                ss >> node_id;
                graph[node_id] = vector<Edge>();  // create node with empty vector of Edges
                all_nodes.push_back(node_id);
            }   
            else if (leader == "edge") {
                ss >> to_node >> cost;
                graph[node_id].push_back(Edge(to_node, cost));  // push Edge to current node_id
            }     
            else if (leader == "data") {
                ss >> data;
                set_node_data(data, node_id); 
            }
        }
} 


// print graph to an output stream ot
//    this is the table of nodes and edges in the graph, not a picture of the game board
//    you can pass an fstream as the output stream
//    cout is the default value of ot--to screen/console
void Graph::display_graph(ostream& ot=cout, string filename="")
{
    int node_id;
    vector<Edge> * edges;
    
    // print the graph in order
    ot << "\nsize " << graph.size() << "\n";
    for (const int node_id : all_nodes) {
        edges = &(graph.at(node_id));
        ot << "node " << node_id << endl;
        ot << "    data " << node_data[node_id] << "\n";
        for (int j = 0; j< (*edges).size(); j++) {       
            ot << "    " << "edge " << 
            edges->at(j).to_node << " " <<  edges->at(j).cost << endl;
        }
    }
}


// ##########################################################################
// #                            class HexBoard
// #  creates a Graph object as a member of HexBoard using one either
// #      method load_board_from_file or make_board
// #  methods for defining and drawing the pictorial representation of the board 
// #  methods for a person to play against this program
// #
// ##########################################################################

class HexBoard
{
public:
    HexBoard() = default;
    ~HexBoard() = default;
    
    
// fields
private:
    // a member of HexBoard that is a Graph object, using "composition" instead of inheritance 
        // the actual graph is created by either method: make_board or load_board_from_file
    Graph hex_graph;                    
    vector<int> &positions = hex_graph.node_data;  // a reference to the graph's node_data

    vector<int> side_one_start;         // indices to the board in the north-south starting border
    vector<int> side_one_finish;        // etc....
    vector<int> side_two_start;
    vector<int> side_two_finish;
    
    vector<RankCol> computer_move_seq;  // history of moves by the computer
    vector<RankCol> player_move_seq;    // history of moves by the human player
    
public:
    int edge_len = 0;
    int max_rank = 0;    // and equals max_col -> so, only need one
    int max_idx = 0;     // maximum linear index
    int move_count = 0;  // number of moves played during the game
    vector<int> &all_nodes = hex_graph.all_nodes;    // maintains sorted list of nodes by linear index
    vector<int> rand_nodes;  // use to shuffle nodes for monte carlo simulation of game moves


// methods
private:
    // METHODS FOR DRAWING THE BOARD
        // return hexboard marker based on value 
        //     and add the spacer lines ___ needed to draw the board
    string symdash(int val, bool last=false)
    {
        string symunit;
        string dot = ".";  // for markers encoded as 0 == empty
        string x = "X";    // for markers encoded as 1
        string o = "O";    // for markers encoded as 2
        string spacer = "___";   // part of drawing the board as ascii characters
        
        if (last) spacer = "";
        
        if (val == EMPTY_HEX)
            symunit = dot + spacer;
        else if (val == PLAYER1_X)
            symunit = x + spacer;
        else if (val == PLAYER2_O)
            symunit = o + spacer;
        else {
            cout << "Error: invalid hexboard value: " << val << endl;
            exit(-1);
        }
        
        return symunit;
    }
    
    // how many spaces to indent each line of the hexboard?
    string lead_space(int rank)
    {
        return string_by_n(" ", rank * 2);
    }
    
    // some string constants used to draw the board
    const string connector = R"( \ /)";
    const string last_connector = R"( \)";
    
    // create sets containing start and finish borders for both sides
    void board_regions()                 // tested OK
    {
        int rank;
        int col;
        
        // yes--we could this all in one loop but, this is much more obvious
        
        for (int rank=1, col=1; col < edge_len+1; col++) {   
            side_one_start.push_back(linear_index(rank, col));
        }
        for (int rank=edge_len, col=1; col < edge_len+1; col++) {
            side_one_finish.push_back(linear_index(rank, col));
        }

        for (int rank=1, col=1; rank < edge_len+1; rank++) {
            side_two_start.push_back(linear_index(rank, col));
        }

        for (int rank=1, col=edge_len; rank < edge_len+1; rank++) {
            side_two_finish.push_back(linear_index(rank, col));
        }
    }

    
public:
    
    // externally defined methods to create a valid Hex game board and graph representation
        // initialize members
        // create graph structure of board by adding all edges
    void make_board(int border_len);  
    void load_board_from_file(string);
    
    // methods for playing game externally defined
    void play_game(bool);
    RankCol prompt_for_person_move(int);
    RankCol computer_move(int, vector<RankCol> &, vector<RankCol> & );
    RankCol random_move();
    RankCol naive_move();
    RankCol prompt_for_computer_move(int);  // used during debugging
    bool is_valid_move(RankCol);
    int who_won();
    

    // classes used to define graph and find longest paths on the board
    friend class Graph;
    friend class Dijkstra;


    // print the ascii board on screen
    void display_board()
    {
        {
            bool last;   // last board value in the rank: true or false?
            
            // format two lines for each rank (except the last)
            for (int rank=1; rank < edge_len+1; rank++) {  
                cout << lead_space(rank);
                for (int col=1; col < edge_len+1; col++) {  
                    last = col < edge_len ? false : true;
                    cout << symdash(get_hex_position(rank, col), last);  // add each column value
                }
                
                cout << endl; // line break for rank
                
                // connector lines to show edges between board positions
                if (rank != edge_len) {
                    cout << lead_space(rank);  // leading spaces for connector line
                    cout << string_by_n(connector, max_rank) << last_connector << endl;
                }
                else {
                    cout << "\n\n";      // last rank: no connector slashes
                }
            }
        }
    }
    
    
    // GAME PLAY METHODS
    void simulate_hexboard_positions()  // utility function creates random board positions
    {                
        int val=0;
        for (int i=0; i < hex_graph.node_data.size(); i++) {
            while (val == EMPTY_HEX) {
                val = rand() % 3;
            }
            hex_graph.node_data[i] = val;
            val = 0;
        }
    }
        
        
    // copy RankCol positions to a set of ints.   caller provides destination set.
    void make_move_set(vector<RankCol> moves, set<int> & cands)
    {
        int node;
        for (auto m : moves) {
            node = linear_index(m);
            cands.insert(node);
        }
    }  
  
    // game play methods use rank and col for board positions
        // rank and col indices are 1-based for end users playing the game
    void set_hex_position(int val, RankCol rc)
    {
        positions[linear_index(rc)] = val;
    }
    
    void set_hex_position(int val, int rank, int col)
    {
        positions[linear_index(rank, col)] = val;
    }

    void set_hex_position(int val, int linear)
    {
        positions[linear] = val;
    }
    
    int get_hex_position(RankCol rc)
    {
        return positions[linear_index(rc)];
    }
    
    int get_hex_position(int rank, int col)
    {
        return positions[linear_index(rank, col)];
    }

    int get_hex_position(int linear)
    {
        return positions[linear];
    }

    // convert rank, col position to a linear index to an array or map
        // graph and minimum cost path use linear indices
        // linear indexes are 0-based to access c++ data structures
    int linear_index(RankCol rc)
    {
        int rank;
        int col;
        
        rank = rc.rank - 1;   // convert from input 1-based indexing to zero-based indexing
        col = rc.col - 1;
        if (rank < edge_len && col < edge_len ) {
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
        rank -= 1;  // convert from input 1-based indexing to zero-based indexing
        col -= 1;
        if (rank < edge_len && col < edge_len ) {
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
            return RankCol(-1,-1);
        } 
    }

    inline bool is_empty(RankCol rc)
    {
        return is_empty(linear_index(rc));
    }
    
    inline bool is_empty(int linear) {
        return positions[linear] == EMPTY_HEX;
    }
    
    inline int count_nodes() const
    {
        return positions.size();
    }
    
    inline int get_node_data(int node) const
    {
        return positions[node];  
    }

};  
// ######################################################
// end class hexboard
// ######################################################


   
void HexBoard::make_board(int border_len)       // initialize board positions
{
    // initialize all members
    edge_len = border_len;
    max_rank = edge_len - 1;
    max_idx = edge_len * edge_len;  // same as size
    
    // REMINDER!!!: rank and col indices are treated as 1-based!
    
    // reserve storage
    positions.reserve(max_idx);
    
    // define the board regions
    board_regions();
    
    // initialize positions: and also node_data because positions is a reference to it
    positions.insert(positions.begin(), max_idx, EMPTY_HEX);
    
    // reserve memory and initialize int for Graph member hex_graph
    hex_graph.graph.reserve(max_idx);
    hex_graph.made_size = max_idx;
    
    // add nodes:  the required hexagonal "tiles" on the board
    // initial values:  all tiles are empty = 0
    for (int i=0; i < max_idx; i++) {
        all_nodes.push_back(i);              // set of nodes used to find paths for both HexBoard and Graph
        hex_graph.graph[i] = vector<Edge>();       // create empty edge list for each tile (aka, node)
        rand_nodes.push_back(i);             // vector of nodes
    }
    
    // add graph edges for adjacent hexes based on the layout of a Hex game board
    // 4 corners                                tested OK
    // upper left
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
        int r=1;
        hex_graph.add_edge(linear_index(r, c), linear_index(r, c-1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r, c+1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c-1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c));
        
        r=edge_len;
        hex_graph.add_edge(linear_index(r, c), linear_index(r, c-1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r, c+1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c));
        hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c+1));
    }
    // east-west edges: constant col, vary rank
    for (int r = 2; r < edge_len; r++) {      
        int c = 1;
        hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c));
        hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c+1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r, c+1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c));
        
        c = edge_len; 
        hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c));
        hex_graph.add_edge(linear_index(r, c), linear_index(r, c-1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c-1));
        hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c));
    }
    
    // interior tiles: 6 edges per hex
    for (int r = 2; r < edge_len; r++) {
        for (int c = 2; c < edge_len; c++) {                
            hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c+1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c+1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c));
            hex_graph.add_edge(linear_index(r, c), linear_index(r+1, c-1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c-1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r-1, c));
        }
    }
}  // end of make_board



void HexBoard::load_board_from_file(string filename)
{
    hex_graph.load_graph_from_file(filename);

    // initialize HexBoard class members
    max_idx = hex_graph.count_nodes();
    edge_len = sqrt(max_idx);
    max_rank = edge_len - 1;

    if (edge_len * edge_len != max_idx) {
        cout << "Error: incorrect size for hexboard. Got size = " << max_idx << endl;
        cout << "Size must have an integer square root for number of tiles in edge of board.\n";
        exit(-1);
    }

    // define the board regions
    board_regions();
} 

// ##########################################################################
// #             end of class HexBoard methods
// ##########################################################################



// ##########################################################################
// #             class Dijkstra
// # find minimum cost path, slightly adapted for playing Hex
// # inputs from class HexBoard make it possible to find a path of markers
// #    on the board from one player's starting edge to the finish edge
// ##########################################################################
class Dijkstra
{
public:
    Dijkstra() = default;    
    ~Dijkstra() = default;
// fields
private:
    set<int> path_nodes;
    unordered_map<int, int, IntHash> path_costs;  // dest. node -> cost to reach 
    unordered_map<int, deque<int>, IntHash> path_sequences;  // dest. node -> path of nodes this node
    
public:
    int start_node;
    
// methods
    public:
    // externally defined methods: effectively the real constructor
    void find_shortest_paths(const Graph &, int, int, bool);
    void find_shortest_paths(const Graph &, int, int, set<int>, bool); 

    
    // print all shortest paths start_node to all other nodes
    friend  ostream& operator<< (ostream& os, Dijkstra & dp)
    {
        for (int node : dp.path_nodes) {
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
    
    // methods to navigate resulting path_sequences
    bool path_sequence_exists(int node) 
    {
        return path_sequences.find(node) != path_sequences.end() ? true : false;
    }
};
// end of class Dijkstra


// method for caller that doesn't prebuild candidate_nodes
void Dijkstra::find_shortest_paths(const Graph &graf, int start_here, int data_filter, bool verbose=false)
{
    set<int> candidate_nodes; 
    int node_val = 0;
    // candidates for the shortest paths must match the value in 'data_filter'
    for (auto node : graf.all_nodes) {    
        node_val = graf.get_node_data(node);
        if (node_val == data_filter) {    
            candidate_nodes.insert(node);   
        }
    }

    find_shortest_paths(graf, start_here, data_filter, candidate_nodes, false);
}


// method for caller that prebuilds appropriate candidate nodes
void Dijkstra::find_shortest_paths(const Graph &graf, int start_here, int data_filter, 
    set<int> candidate_nodes, bool verbose=false)
{
    
    int num_nodes = graf.count_nodes();
    
    // reserve memory for members
    path_costs.reserve(num_nodes);
    path_sequences.reserve(num_nodes);

    start_node = start_here;
    
    // local variables
    const int inf = INT_MAX;
    int neighbor_node=0;
    int current_node=start_node;
    int prev_node=0;
    int min = inf;
    int tmp_cost = 0;
    int node_val = -1;
    deque<int> tmpsequence; 
    // set<int> candidate_nodes;   // initialized below with only valid nodes
    vector<Edge>  neighbors;   // end_node and cost for each neighbor node
    unordered_map<int, int, IntHash> previous;
        previous.reserve(num_nodes);
        
    // candidates for the shortest paths must match the current player in 'data_filter'
        for (auto node : candidate_nodes) {    
            node_val = graf.get_node_data(node);
            if (node_val == data_filter) {    
                // candidate_nodes.insert(node);   
                path_costs[node] = inf;   // initialize costs to inf for Dijkstra alg.
            }
        }
    
    path_costs[start_node] = 0;
    previous[start_node] = -1;
    
    // algorithm loop
    while (!(candidate_nodes.empty()))
        {
            // current_node begins as start_node
            
            if (verbose)
                cout << "\ncurrent_node at top of loop " << current_node << endl;
            
            if (graf.get_node_data(current_node) != data_filter) break;

            neighbors = graf.get_neighbors(current_node, data_filter, path_nodes);  //, path_nodes);  // vector<Edge>
            for (auto & neighbor : neighbors) {    // neighbor is an Edge
                neighbor_node = neighbor.to_node;
                tmp_cost = path_costs[current_node] + neighbor.cost; // update path_costs for neighbors of current_node
                if (tmp_cost < path_costs[neighbor_node]) {
                    path_costs[neighbor_node] = tmp_cost;
                    previous[neighbor_node] = current_node;
                }
            }
            
            if (current_node == start_node && neighbors.empty()) break;  // start_node is detached from all others
            
            if (verbose) {
                //  for debugging shortest path algorithm
                cout << "**** STATUS ****\n";
                cout << "  current_node " << current_node << endl;
                cout << "  neighbors\n";
                cout << neighbors << endl;
                cout << "  path_nodes\n";
                cout << "    " << path_nodes << endl;
                cout << "  candidate_nodes\n";
                cout << "    " <<  candidate_nodes << endl;
                cout << "  previous\n";
                cout << previous << endl;
                cout << "  path cost\n";
                cout << path_costs << endl;
            }
            
            path_nodes.insert(current_node);
            candidate_nodes.erase(current_node);  // we won't look at this node again
            
            // pick next current_node based on minimum distance, which will be a neighbor whose cost
            // has been updated from infinity to the network edge cost
            min = inf; 
            for (const int tmp_node : candidate_nodes) {  // always O(n)
                if (path_costs[tmp_node] < min) {
                    min = path_costs[tmp_node];
                    current_node = tmp_node;
                }
            }
            
            if (min == inf) {   // none of the remaining nodes are reachable
                break;
            }
            
            if (verbose) cout << "  current node at bottom: " << current_node << endl;
        }
    
    // build sequences by walking backwards from each dest. node to start_node 
    for (const auto walk_node : path_nodes)
        {
            prev_node = walk_node;
            while (prev_node != start_node) {
                tmpsequence.push_front(prev_node);  // it's a deque
                prev_node = previous[prev_node];
            } ;
            tmpsequence.push_front(prev_node);
            path_sequences[walk_node] = tmpsequence;
            
            tmpsequence.clear();  // IMPORTANT: clear before reusing to build a new sequence!
        }
    
    if (verbose) cout << *this << endl;   // prints the nodes and shortest path_sequences to each node
}


// ##########################################################################
// #             end of class Dijkstra methods
// ##########################################################################


// ##########################################################################
// #             Class HexBoard game playing methods
// ##########################################################################


// the program makes a random move
RankCol HexBoard::random_move()
{
    RankCol rc;
    int maybe;  // candidate node to place a marker on
    
    shuffle(rand_nodes.begin(), rand_nodes.end(), randgen);
    
    for (int i=0; i < max_idx; i++) {
        maybe = rand_nodes[i];
        if (is_empty(maybe)) {
            rc = rank_col_index(maybe);
            break;
        }
    }
    if (rc.rank == 0 && rc.col == 0) {  // never found an empty position->no possible move
        rc.rank = -1; rc.col = -1;  // no move
    }
    
    return rc;
}

// use for testing only
RankCol HexBoard::prompt_for_computer_move(int side)
{
    RankCol rc;
    int rank;
    int col;
    int val;
    bool valid_move = false;
    char pause;
    
    while (!valid_move)
        {
            cout << "Enter a move for the computer.\n";
            cout << "Enter the rank (the row, using Chess terminology)... ";
            
            cin >> rank;
            
            cout << "Enter the column... ";
            
            cin >> col;
            
            rc.rank = rank; rc.col = col;
            valid_move = is_valid_move(rc);
        }
    set_hex_position(side, rc);
        
    return rc;
}

// the program makes a naive move to extend its longest path
RankCol HexBoard::naive_move()
{
    RankCol rc;
    RankCol prev_move;
    int prev_move_linear;
    vector<int> neighbor_nodes;
    char pause;
    
    
    if (computer_move_seq.empty()) {
        shuffle(side_two_start.begin(), side_two_start.end(), randgen);
        for (int maybe : side_two_start) {
            if (is_empty(maybe)) {
                rc = rank_col_index(maybe);
                return rc;
            }
        }
    }
    else {
        prev_move = computer_move_seq.back();
        prev_move_linear = linear_index(prev_move);
        
        neighbor_nodes = hex_graph.get_neighbor_nodes(prev_move_linear, EMPTY_HEX);   
        
        if (neighbor_nodes.empty()) return random_move();
        
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


RankCol HexBoard::computer_move(int side, vector<RankCol> & player_move_seq, vector<RankCol> & computer_move_seq)
{
    RankCol rc;
    
//  rc = random_move();   // this is WORSE than naive...

    rc = naive_move();

    return rc;
}


RankCol HexBoard::prompt_for_person_move(int side)
{
    RankCol rc;
    int rank;
    int col;
    int val;
    bool valid_move = false;
    
    while (!valid_move)
    {
        cout << "You go first playing X markers. The computer goes second playing O markers.\n";
        cout << "Enter a move in an empty position that contains '.'" << endl;
        cout << "(A Note for c++ programmers: end-users use 1-indexing, so that's what we use...)\n";
        cout << "Enter -1 to quit..." << endl;
        cout << "Enter the rank (the row, using Chess terminology)... ";
        
        cin >> rank;
        
        if (rank == -1) {
            rc.rank=-1; rc.col = -1;
            return rc;
        }
        
        if (rank == -5) {   // hidden command to write the current board positions to a file
            // prepare output file
            string filename = "Board Graph.txt";
            fstream outfile;
            outfile.open(filename, ios::out);  // open a file to perform write operation using file object
            if (!(outfile.is_open())) {
                cout << "Error opening file: " << filename << " Terminating.\n";
                exit(-1);
            }
            hex_graph.display_graph(outfile);

        }
        
        cout << "Enter the column... ";
        
        cin >> col;
        
        if (col == -1) {
            rc.rank=-1; rc.col = -1;
            return rc;
        }
        
        rc.rank = rank; rc.col = col;
        valid_move = is_valid_move(rc);
    }

    return rc;
}


bool HexBoard::is_valid_move(RankCol rc)
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
    else if (get_hex_position(rc) != 0) {
        valid_move = false;
        msg = not_empty;
    }
    
    cout << msg;
    
    return valid_move;
}


int HexBoard::who_won() 
{
    int winner=0;
    int finish_hex;
    int start_hex;
    set<int> candidates;
            
    // test for side one victory
    // for each hex in side_one_finish, find a path that ends in side_one_start
    for (int finish_hex : side_one_finish) {
        if (get_hex_position(finish_hex) != PLAYER1_X) continue;
        Dijkstra paths;
        
        
        make_move_set(player_move_seq, candidates);  // has to be a copy because we need to keep player_move_seq
                                                     //    and we're going to remove items from candidates
        
        paths.find_shortest_paths(hex_graph, finish_hex, PLAYER1_X, candidates);   // find a path that starts from the finish border
        for (int start_hex : side_one_start) {
            if (paths.path_sequence_exists(start_hex)) {         // to the starting border
                winner = PLAYER1_X;
                return winner;
            }
        }
    }
    
    candidates.clear();
    
    // test for side two victory
    // for each hex in side_two_finish, find a path that ends in side_two_start
    for (int finish_hex : side_two_finish) {
        if (get_hex_position(finish_hex) != PLAYER2_O) continue;
        Dijkstra paths;
        
        make_move_set(computer_move_seq, candidates);
        
        paths.find_shortest_paths(hex_graph, finish_hex, PLAYER2_O, candidates);  
        for (int start_hex : side_two_start) {
            if (paths.path_sequence_exists(start_hex)) {
                winner = PLAYER2_O;
                return winner;
            }
        }
    }
    
    return winner;  // will always be 0
}

void HexBoard::play_game(bool simulate=false)
{
    RankCol rc;            // person's move
    RankCol computer_rc;   // computer's move
    char pause;
    bool valid_move;
    bool end_game = false;
    int person_marker = PLAYER1_X;
    int computer_marker = PLAYER2_O;
    int winning_side;
    
    if (simulate) {
        simulate_hexboard_positions();
    }
    else {
        while (true)                // move loop
        {
            cout << "\n\n";
            display_board();
            
            rc = prompt_for_person_move(person_marker);
            if (rc.rank == -1) {
                cout << "Game over! Come back again...\n";
                end_game = true;
                break;
            }
            if (end_game) break;

            set_hex_position(person_marker, rc);
            player_move_seq.push_back(rc);

            display_board();
            computer_rc = computer_move(computer_marker, player_move_seq, computer_move_seq);
            set_hex_position(computer_marker, computer_rc);
            computer_move_seq.push_back(computer_rc);
            
            move_count++;
            
            clear_screen();
            
            // test for a winner
            if (move_count >= edge_len) {
                winning_side = who_won();  // result is 0, 1, or 2
                if (winning_side)
                    {
                        cout << "We have a winner. " 
                        << (winning_side == 1 ? "You won. Congratulations!" : " The computer beat you )-:")
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


int main(int argc, char *argv[]) 
{
    srand(time(0));
    
    int size=5;
    
    if (argc == 2)
        size = atoi(argv[1]);
    
    if (size > 11) {
        cout << "Your board size might be a little large: try between 5 and 11.\n";
        cout << "Let's go ahead and play a tiny game with a 5 x 5 board.\n";
        size = 5;
    }
    else if (size < 5) {
        cout << "Size should be from 5 through 11.\n";
        cout << "Let's go ahead and play a tiny game with a 5 x 5 board.\n";
        size = 5;
    }
    
    HexBoard hb;
    hb.make_board(size);
    hb.play_game();
    
    return 0;
}

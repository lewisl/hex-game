// Draw a hexboard and play the game of hex
// Run as ./hex 5 where the single argument is the number of hex's on each border of the board
// For now, the computer plays randomly and a human wins easily
// Programmer: Lewis Levin
// Date: April 2023

#include <iostream>
#include <stdlib.h>
#include <random>
#include <vector>
#include <deque>            // sequence of nodes in a path between start and destination
#include <unordered_map>    // container for definition of Graph, costs, previous nodes for Dijkstra
#include <set>              // hold nodes for shortest path algorithm
#include <fstream>          // to write graph to file and read graph from file
#include <sstream>          // to use stringstream to parse inputs from file

using namespace std;


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

ostream& operator<< (ostream& os, unordered_map<int, int> & um)
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

// rank and col on the hexboard to address a hexagon
    // rank and col are seen by the human player so we use 1-based indexing
    // the linear_index conversion function handles this
struct RankCol
{
    int rank;
    int col;

    RankCol(int rank=1, int col=1) : rank(rank), col(col) {}
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



// ##########################################################################
// #                            class HexBoard
// #  positions: data structure for holding the piece positions on the board
// #  graph: data structure the graph of hexagons on the board
// #  make_board, load_board_from_file: methods to define graph representation
// #  methods for defining and drawing the pictorial representation of the board 
// #  methods for game play: access and set marker at each position, etc.
// #
// ##########################################################################
class HexBoard
{
    
    // no explicit constructor: compiler default constructor makes everything empty or 0.
    // use explicit methods to initialize members or load from file
    
private:
    vector<int> positions;                       // board positions of pieces in [0, 1, 2]
    unordered_map<int, vector<Edge>> graph;      // hexboard as a graph
    vector<int> side_one_start;                     // positions in the north-south starting border
    vector<int> side_one_finish;
    vector<int> side_two_start;
    vector<int> side_two_finish;
    
    // add an edge to a node; add only one outbound edge
    // make_board calls for all the nodes and has logic for determining required edges
    inline void add_edge(int x, int to)  // no cost input as cost defaults to 1 for Edge
    {
        graph[x].push_back(Edge(to));  //  add only outbound edge, other hex will add reciprocal edge
    }

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
        
        if (val == 0)
            symunit = dot + spacer;
        else if (val == 1)
            symunit = x + spacer;
        else if (val == 2)
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
    
    int edge_len = 0;
    int max_rank = 0;   // == max_col -> so, only need one
    int max_idx = 0;    // maximum linear index
    set<int> all_nodes;    // maintains sorted list of nodes by linear index
    vector<int> rand_nodes;  // use to shuffle nodes for monte carlo simulation of game moves
    
    // externally defined methods to create a valid Hex game board and graph representation
        // initialize members
        // create graph structure of board by adding all edges
    void make_board(int border_len);  
    void load_board_from_file(string);
    
    // methods for playing game externally defined
    friend void play_game(void);
    RankCol prompt_for_person_move(int);
    RankCol computer_move(int);      // wrapper for computer moves used in play_game
    RankCol random_move();
    RankCol naive_move();
    RankCol prompt_for_computer_move(int);  // used during debugging
    bool is_valid_move(RankCol);
    friend int who_won(HexBoard &);
    
    
    // externally defined public methods
    void display_graph(ostream&, string);  

    // class used to find longest paths on the board
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
        for (int i=0; i < positions.size(); i++) {
            while (val == 0) {
                val = rand() % 3;
            }
            positions[i] = val;
            val = 0;
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

    
    // get the neighbors of a node as a vector of edges
    inline vector<Edge> get_neighbors(int current_node)  
    {
        return graph[current_node];  //the value type of graph is vector<Edge> and holds the neighbors
    }
    
    // get the neighbors that match the select data--the player whose marker is there
    inline vector<Edge> get_neighbors(int current_node, int data_filter)  
    {
        vector<Edge> ve;
        for (auto e : graph[current_node]) {
            if (positions[e.to_node] == data_filter) {
                ve.push_back(e);
            }
        }
        return ve;
    }
    
    // get the neighbors that match the select data and are not in the exclude set
    inline vector<Edge> get_neighbors(int current_node, int data_filter, set<int> exclude_set)  
    {
        vector<Edge> ve;
        int test_node;
        for (auto e : graph[current_node]) {
            test_node = e.to_node;
            if (positions[test_node] == data_filter && !is_in(test_node, exclude_set)) {
                ve.push_back(e);
            }
        }
        return ve;
    }
    
    inline bool is_empty(RankCol rc)
    {
        return is_empty(linear_index(rc));
    }
    
    inline bool is_empty(int linear) {
        return positions[linear] == 0;
    }
    
    int count_nodes()
    {
        return positions.size();
    }
    
    int get_node_data(int node)
    {
        return positions[node];  
    }


};  
// ######################################################
// end class hexboard
// ######################################################


// for an empty HexBoard
   // initialize all members
void HexBoard::make_board(int border_len)       // initialize board positions
{
    edge_len = border_len;
    max_rank = edge_len - 1;
    max_idx = edge_len * edge_len;  // same as size
    
    // REMINDER!!!: rank and col indices are treated as 1-based!
    
    // reserve storage
    graph.reserve(max_idx);
    positions.reserve(max_idx);
    
    // define the board regions
    board_regions();
    
    // initialize positions
    positions.insert(positions.begin(), max_idx, 0);
    
    // add nodes:  the required hexagonal "tiles" on the board
    // initial values:  all tiles are empty = 0
    for (int i=0; i < max_idx; i++) {
        all_nodes.insert(i);             // set of nodes used to find paths
        graph[i] = vector<Edge>();       // create empty edge list for each tile (aka, node)
        rand_nodes.insert(rand_nodes.begin(), i);   // vector of nodes
    }
    
    // add graph edges for adjacent hexes based on the layout of a Hex game board
    // 4 corners                                tested OK
    add_edge(linear_index(1, 1), linear_index(2, 1));  
    add_edge(linear_index(1, 1), linear_index(1, 2));
    add_edge(linear_index(edge_len, edge_len), linear_index(edge_len, max_rank));  
    add_edge(linear_index(edge_len, edge_len), linear_index(max_rank, edge_len));
    add_edge(linear_index(1, edge_len), linear_index(1, max_rank));
    add_edge(linear_index(1, edge_len), linear_index(2, edge_len));
    add_edge(linear_index(1, edge_len), linear_index(2, max_rank));
    add_edge(linear_index(edge_len, 1), linear_index(max_rank, 1));
    add_edge(linear_index(edge_len, 1), linear_index(edge_len, 2));
    add_edge(linear_index(edge_len, 1), linear_index(max_rank, 2));
    
    // 4 borders (excluding corners)  4 edges per node. 
    // north-south edges: constant rank, vary col
    for (int c = 2; c < edge_len; c++) {     
        int r=1;
        add_edge(linear_index(r, c), linear_index(r, c-1));
        add_edge(linear_index(r, c), linear_index(r, c+1));
        add_edge(linear_index(r, c), linear_index(r+1, c-1));
        add_edge(linear_index(r, c), linear_index(r+1, c));
        
        r=edge_len;
        add_edge(linear_index(r, c), linear_index(r, c-1));
        add_edge(linear_index(r, c), linear_index(r, c+1));
        add_edge(linear_index(r, c), linear_index(r-1, c));
        add_edge(linear_index(r, c), linear_index(r-1, c+1));
    }
    // east-west edges: constant col, vary rank
    for (int r = 2; r < edge_len; r++) {      
        int c = 1;
        add_edge(linear_index(r, c), linear_index(r-1, c));
        add_edge(linear_index(r, c), linear_index(r-1, c+1));
        add_edge(linear_index(r, c), linear_index(r, c+1));
        add_edge(linear_index(r, c), linear_index(r+1, c));
        
        c = edge_len; 
        add_edge(linear_index(r, c), linear_index(r-1, c));
        add_edge(linear_index(r, c), linear_index(r, c-1));
        add_edge(linear_index(r, c), linear_index(r+1, c-1));
        add_edge(linear_index(r, c), linear_index(r+1, c));
    }
    
    // interior tiles: 6 edges per hex
    for (int r = 2; r < edge_len; r++) {
        for (int c = 2; c < edge_len; c++) {                
            add_edge(linear_index(r, c), linear_index(r-1, c+1));
            add_edge(linear_index(r, c), linear_index(r, c+1));
            add_edge(linear_index(r, c), linear_index(r+1, c));
            add_edge(linear_index(r, c), linear_index(r+1, c-1));
            add_edge(linear_index(r, c), linear_index(r, c-1));
            add_edge(linear_index(r, c), linear_index(r-1, c));
        }
    }
    
    
}  // end of make_board


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
void HexBoard::load_board_from_file(string filename)
{
    // prepare input file
    ifstream infile;
    infile.open(filename);  // open a file to perform read operation using file object
    if (!(infile.is_open())) {
        cout << "Error opening file: " << filename << " Terminating.\n";
        exit(-1);
    }
    
    // define the board regions
    board_regions();
        
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
                ss >> max_idx;
                
                edge_len = sqrt(max_idx);
                
                if (edge_len * edge_len != max_idx) {
                    cout << "Error: incorrect size for hexboard. Got size = " << max_idx << endl;
                    cout << "Size must have an integer square root for number of tiles in edge of board.\n";
                    exit(-1);
                }
                
                max_rank = edge_len - 1;
                
                // reserve storage
                graph.reserve(max_idx);
                positions.reserve(max_idx);
                
                // initialize all the positions as zero
                positions.insert(positions.begin(), max_idx, 0);
            }
            else if (leader == "node") {
                ss >> node_id;
                graph[node_id] = vector<Edge>();  // create node with empty vector of Edges
                all_nodes.insert(node_id);
            }   
            else if (leader == "edge") {
                ss >> to_node >> cost;
                graph[node_id].push_back(Edge(to_node, cost));  // push Edge to current node_id
            }     
            else if (leader == "data") {
                ss >> data;
                set_hex_position(data, node_id); 
            }
        }
} 


// print graph to an output stream ot
//    this is the table of nodes and edges in the graph, not a picture of the game board
//    you can pass an fstream as the output stream
//    cout is the default value of ot--to screen/console
void HexBoard::display_graph(ostream& ot=cout, string filename="")
{
    int node_id;
    vector<Edge> * edges;
    
    // print the graph in order
    ot << "\nsize " << graph.size() << "\n";
    for (const int node_id : all_nodes) {
        edges = &(graph.at(node_id));
        ot << "node " << node_id << endl;
        ot << "    data " << positions[node_id] << "\n";
        for (int j = 0; j< (*edges).size(); j++) {       
            ot << "    " << "edge " << 
            edges->at(j).to_node << " " <<  edges->at(j).cost << endl;
        }
    }
}


// ##########################################################################
// #             end of class HexBoard methods
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
    Dijkstra() = default;   
    ~Dijkstra() = default;
    int start_node;
    
    // externally defined methods: effectively the real constructor
    void find_shortest_paths(HexBoard &, int, int, bool);
    
    // friends for playing game
    friend int who_won(const HexBoard &); 

    
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
    
    
    // note: always test first for the existence of the finish_node's path_sequence
    bool is_in_path_sequence(int search_node, int finish_node)
    {
        bool ret = false;
        if (path_sequences.find(finish_node) != path_sequences.end()) {
            if (path_sequences.find(search_node) != path_sequences.end()) {
                ret = true;
            }
        }
        
        return ret;
    }
    
};
// end of class Dijkstra

void Dijkstra::find_shortest_paths(HexBoard &graf, int start_here, int data_filter, bool verbose=false)
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
    set<int> candidate_nodes(graf.all_nodes);   // initialize with copy constructor
    vector<Edge>  neighbors;   // end_node and cost for each neighbor node
    unordered_map<int, int> previous;
        previous.reserve(num_nodes);
        
    // if using data, eliminate nodes that don't match data selection criteria from candidate list
        for (auto node : graf.all_nodes) {    // iterate nodes #2 through end
            node_val = graf.get_node_data(node);
            if (node_val != data_filter) {
                candidate_nodes.erase(node);   // eliminate from candidate nodes
            }
        }
    
    for (auto tmp_node : candidate_nodes) {
        path_costs[tmp_node] = inf;
    }
    
    path_costs[start_node] = 0;
    previous[start_node] = -1;
    
    // algorithm loop
    while (!(candidate_nodes.empty()))
        {
            // current_node begins as start_node
            
            if (graf.get_node_data(current_node) != data_filter) break;
            
            neighbors = graf.get_neighbors(current_node, data_filter); //, path_nodes);  // vector<Edge>
            for (auto & neighbor : neighbors) {    // neighbor is an Edge
                neighbor_node = neighbor.to_node;
                tmp_cost = path_costs[current_node] + neighbor.cost; // update path_costs for neighbors of current_node
                if (tmp_cost < path_costs[neighbor_node]) {
                    path_costs[neighbor_node] = tmp_cost;
                    previous[neighbor_node] = current_node;
                }
            }
            
            if (current_node == start_node && neighbors.empty()) break;
            
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
            
            if (min == inf) {
                break;
                current_node = *candidate_nodes.begin();    // arbitrarily pick a node
            }
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
            
            tmpsequence.clear();  // IMPORTANT: clear before reusing to build new sequence!
        }
}


// ##########################################################################
// #             end of class Dijkstra methods
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
    
    // not implemented yet
    
    return rc;
}

RankCol prompt_for_person_move(HexBoard & board, int side);  // declare ahead of function computer_move

RankCol HexBoard::computer_move(int side)
{
    RankCol rc;
    int rank;
    int col;
    int val;
    bool valid_move = false;
    char pause;
    
    
    rc = random_move();   // this is WORSE than naive...
    if (rc.rank > 0) {
        set_hex_position(side, rc);
    }

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
            display_graph(outfile);

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


int who_won(HexBoard &hb) 
{
    int winner=0;
    int finish_hex;
    int start_hex;
            
    // test for side one victory
    // for each hex in side_one_finish, find a path that ends in side_one_start
    for (int finish_hex : hb.side_one_finish) {
        if (hb.get_hex_position(finish_hex) != 1) continue;
        Dijkstra paths;
        paths.find_shortest_paths(hb, finish_hex, 1);   // 
        for (int start_hex : hb.side_one_start) {
            if (paths.path_sequence_exists(start_hex)) {
                winner = 1;
                return winner;
            }
        }
    }
    
    // test for side two victory
    // for each hex in side_two_finish, find a path that ends in side_two_start
    for (int finish_hex : hb.side_two_finish) {
        if (hb.get_hex_position(finish_hex) != 2) continue;
        Dijkstra paths;
        paths.find_shortest_paths(hb, finish_hex, 2);   // 
        for (int start_hex : hb.side_two_start) {
            if (paths.path_sequence_exists(start_hex)) {
                winner = 2;
                return winner;
            }
        }
    }
    
    return winner;  // will always be 0
}

void play_game(HexBoard & hb, bool simulate=false)
{
    RankCol rc;            // person's move
    RankCol computer_rc;   // computer's move
    bool valid_move;
    bool end_game = false;
    int person_marker = 1;
    int computer_marker = 2;
    int move_count=0;
    int winning_side;
    
    if (simulate) {
        hb.simulate_hexboard_positions();
    }
    else {
        while (true)                // move loop
        {
            cout << "\n\n";
            hb.display_board();
            
            valid_move = false;
            while (!valid_move) {
                rc = hb.prompt_for_person_move(person_marker);
                if (rc.rank == -1) {
                    cout << "Game over! Come back again...\n";
                    end_game = true;
                    break;
                }
                valid_move = hb.is_valid_move(rc);
            }
            if (end_game) break;
            move_count++;
            
            
            
            hb.set_hex_position(person_marker, rc);
            clear_screen();
            hb.display_board();
            computer_rc = hb.computer_move(computer_marker);
            
            clear_screen();
            
            // test for a winner
            if (move_count >= hb.edge_len) {
                winning_side = who_won(hb);  // result is 0, 1, or 2
                if (winning_side)
                    {
                        cout << "We have a winner. " 
                        << (winning_side == 1 ? "You won. Congratulations!" : " The computer beat you )-:")
                        << "\nGame over. Come back and play again\n\n"; 
                        hb.display_board();
                        break;
                    }
            }
            
            cout << "Your move at " << rc << " is valid.\n";
            cout << "The computer moved at " << computer_rc << "\n";
            
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
    if (size < 5) {
        cout << "Size should be from 5 through 11.\n";
        cout << "Let's go ahead and play a tiny game with a 5 x 5 board.\n";
        size = 5;
    }
    
    HexBoard hb;
    hb.make_board(size);
    play_game(hb);
    
    return 0;
}
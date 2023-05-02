/* Draw a hexboard and play the game of hex
Run as ./hex 7 or ./hex 5 1000
Default size is 5; default trials = 1000 so running ./hex is a quick game.
Two arguments: first is board edge length;
               second is number of trials for each position assessed by simulation.
The computer uses simulation to choose its moves,  but a human often wins.
Please compile with -std=c++14 given features I've used.
Note that there are 2 classes defined in one big file.  In multi-file compile and build,
the file would be split to include the 2 class definitions each in their own file,
with a small file containing int main(argc, *char[argv]) to run the game.
Programmer: Lewis Levin Date: April 2023
*/

#include <chrono> // for performance timing
#include <deque> // sequence of nodes in a path between start and destination
#include <fstream> // to write graph to file and read graph from file
#include <iostream>
#include <random>
#include <sstream> // to use stringstream to parse inputs from file
#include <stdlib.h> // for atoi()
#include <string>
#include <unordered_map> // container for definition of Graph
#include <vector>

using namespace std;

// send control codes to console to clear the screen
// not guaranteed to work on all OS'es.  Works on MacOs.
void clear_screen() { cout << u8"\033[2J"; }


// print key and values of an unordered_map
template <class T> ostream &operator<<(ostream &os, const unordered_map<int, int, T> &um)
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

// print elements of a deque for various element types
template <class T>
ostream &operator<<(ostream &os, const deque<T> &dq)
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

// print elements of a vector for various element types
template <class T> ostream &operator<<(ostream &os, const vector<T> &dq)
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

std::string tolower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return tolower(c); });
    return result;
}

// other non-class functions

// test if value is in vector with trivial linear search for various primitive element types
template <typename T>
bool is_in(T val, vector<T> vec)
{
    auto it = find(vec.begin(), vec.end(), val);
    return it != vec.end();
}

// test if value of char or string is in a string
template <typename T> // should be char or string
bool is_in(T val, const string & test_string)
{

    return (test_string.find(val) != std::string::npos);
}

// test if value is in deque with trivial linear search
template <typename T>
bool is_in(T val, deque<T> deq)
{
    auto it = find(deq.begin(), deq.end(), val);
    return it != deq.end();
}

// compare to a single value not wrapped in a container
template<typename T>
bool is_in(T val, T one) { return val == one; }

/** class Timing
a simple way to time the execution of segments of code
ex:
      Timing this_timer;
      this_timer.start();  // starts this_timer and saves the time
      < bunch-o-code >
      this_timer.cum();    // stops this_timer and adds the time since start to cumulative duration of this_timer
      cout << "This code took " << this_timer.show() << " seconds\n";

      other methods:
      this_timer.stop();   // stops this_timer and saves the time: used by cum
      this_timer.ticks();  // returns the time between the previous start and stop: used by cum
      this_timer.reset();  // resets start, stop, and duration (duration is updated and returned by cum),
                           // so that you can re-use this_timer.  You could also re-initialize it: Timing this_timer;
*/
class Timing {
  private:
    std::chrono::time_point<std::chrono::steady_clock> begint;
    std::chrono::time_point<std::chrono::steady_clock> endt;
    double duration = 0.0;

  public:
    void start() { begint = std::chrono::steady_clock::now(); }

    void stop() { endt = std::chrono::steady_clock::now(); }

    double ticks() { 
        if (endt > begint) 
            return chrono::duration_cast<std::chrono::duration<double>>(endt - begint).count();
        else return 0.0; 
    }

    void cum() { stop(); duration += ticks(); }

    void reset()
    {
        std::chrono::time_point<std::chrono::steady_clock> begint;
        std::chrono::time_point<std::chrono::steady_clock> endt;
        double duration = 0.0;
    }

    double show() { return duration; }
};


/* 
##########################################################################
#                            class Graph
#  graph: data structure holding nodes and their edge
#  node_data: data structure for holding data value at each node
#  load_graph_from_file: method to define graph representation
#  this class is not really bound to hex in any way and can be used
#    for other graph applications.
##########################################################################
*/
template <typename T_data>   // T_data can be various primitive data types
class Graph {
  public:
    Graph<T_data>() = default;
    ~Graph<T_data>() = default;

    vector<T_data> node_data; // holds Data values of all nodes
        
    // holds an edge for a starting node: to node, cost to the neighbor
    // doesn't include starting node because that is part of the graph container
    // all of the edges of a node are held in a vector of Edges
    struct Edge {
        int to_node; // use linear index to hexboard
        int cost; // default to cost=1 => should NOT change this when creating edges

        Edge(int to_node = 0, int cost = 1) : to_node(to_node), cost(cost) {}
    };

  private:
    unordered_map<int, vector<Edge>> graph;

  public:
    void set_storage(int size)
    {
        graph.reserve(size);
        graph.max_load_factor(0.8);
        node_data.reserve(size);
    }

    // output an Edge in an output stream
    friend ostream &operator<<(ostream &os, const Edge &e)
    {
        os << "  to: " << e.to_node << " cost: " << e.cost << endl;
        return os;
    }

    // output a vector of edges: used in the graph definition for each node
    friend ostream &operator<<(ostream &os, const vector<Edge> &ve)
    {
        for (auto const &e : ve) {
            os << e << endl;
        }
        return os;
    }

    int count_nodes() const { return graph.size(); }

    void initialize_data(const T_data val, const int size) { node_data.insert(node_data.begin(), size, val); };

    void set_node_data(const T_data val, const int idx) { node_data[idx] = val; }

    T_data get_node_data(int idx) const { return node_data[idx]; }

    // get the neighbors of a node as a vector of edges
    const vector<Edge> get_neighbors(const int current_node) const
    {
        return graph.at(current_node); // the value type of graph is
            // vector<Edge> and holds the neighbors
    }

    // get the neighbors that match the select data--the player whose marker is
    // there
    vector<Edge> get_neighbors(const int current_node, const T_data data_filter) const
    {
        vector<Edge> ve;
        for (const auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter) {
                ve.push_back(e);
            }
        }
        return ve;
    }

    // get the neighbors that match the select data and are not in the exclude
    // set, deque or vector
    template <typename Container>
    vector<Edge> get_neighbors(const int current_node, const T_data data_filter, const Container &exclude_set) const
    {
        vector<Edge> vr;
        for (const auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(e.to_node, exclude_set))
                vr.push_back(e);
        }
        return vr; // in c++14 this returns an rvalue reference so the caller
            // moves the returned value to a vector
    }

    // get the neighbor_nodes as a vector of nodes instead of the edges
    vector<int> get_neighbor_nodes(const int current_node, const T_data data_filter) const
    {
        vector<int> neighbor_nodes;
        for (const auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter)
                neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }

    // get the neighbor_nodes that match the filter while excluding a set or vector of nodes
    template <typename Container>
    vector<int> get_neighbor_nodes(const int current_node, const T_data data_filter, const Container &exclude) const
    {
        vector<int> neighbor_nodes;
        for (const auto e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(e.to_node, exclude))
                neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }

    void add_edge(const int node) // empty Edge container
    {
        graph.emplace(node, vector<Edge>{});
    }

    // with to_node and cost
    void add_edge(const int node, const int y, const int cost = 1, const bool bidirectional = false)
    {
        if (graph.find(node) != graph.end()) {
            if (graph.find(y) != graph.end()) {
                for (auto edge : graph[node]) { // iterate over edges of node
                    if (y == edge.to_node) {
                        return; // to_node y already exists as an edge of node
                    }
                }
                graph[node].push_back(Edge(y, cost));
                if (bidirectional)
                    graph[y].push_back(Edge(node, cost));
            }
            else {
                cout << "node " << y << " not found in graph.";
            }
        }
        else {
            cout << "node " << node << " not found in graph.";
        }
    }

    /** 
    display_graph
    Print graph to an output stream ot.
    This is the table of nodes and edges in the graph, not a picture of the
    graph. It is identical to the format used by load_graph_from_file. You can 
    pass an fstream as the output stream to save the graph in a text file.
    cout is the default value of ot to print to console.
    */
    void display_graph(ostream &ot = cout, bool to_file=false) const
    {
        vector<int> nodes_sorted(graph.size());

        // copy and sort the keys in graph
        std::transform(graph.begin(), graph.end(), nodes_sorted.begin(),
                       [](const auto &pair) { return pair.first; });
        std::sort(nodes_sorted.begin(), nodes_sorted.end());

        ot << "\nsize " << graph.size() << "\n";
        for (const auto node : nodes_sorted) { // just an int node
            ot << "node " << node << endl;
            ot << "    data " << get_node_data(node) << endl;
            auto & ve = graph.at(node);     // vector<Edge>
            for (const auto & edge : ve) {  // Edge in vector<Edge>
                ot << "    "
                   << "edge " << edge.to_node << " " << edge.cost << endl;
            }
        }
    }
    
    // note: we don't use this for the monte carlo simulation, but it's good for testing
    /*** load_graph_from_file
        Read a graph file to initialize a graph using the format of this example:
        size 4        // optional: if used, will check if it matches actual number of input nodes
        node 0        // node must be positive integer; not required to be consecutive
            data 0    // data value at this position (can be used to represent hex markers)
            edge 2 3  // edge is to_node, cost. No error checking so to_node must exist
        node 1
            data 1
            edge 1 4
            edge 3 5
        node 2
            data 1
            edge 1 4
            edge 3 5
        node 3
            data 1
            edge 1 4
            edge 3 5

        Note: in this format, edges are assumed to be directional. If you want
        bidirectional (aka, non-directional edges that go both ways) then you have to include 2
        reciprocal edges.
    */
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
        size_t node_id, to_node;
        int cost;
        int tmp_size = 0;   // only used locally here
        int input_data; // as read from text file
        T_data data;

        // loads data into graph and positions
        while (getline(infile, linestr)) {
            stringstream ss{linestr};
            ss >> leader; // first word or text up to first white space

            if (leader == "size") { // define size constants and reserve memory
                ss >> tmp_size;

                // reserve storage
                set_storage(tmp_size);
                // graph.reserve(tmp_size);
                // graph.max_load_factor(0.8);
                // node_data.reserve(tmp_size);
            }
            else if (leader == "node") {
                ss >> node_id;
                graph[node_id] = vector<Edge>{}; // create node with empty vector of Edges
            }
            else if (leader == "edge") {
                ss >> to_node >> cost;
                graph[node_id].push_back(Edge(to_node, cost)); // push Edge to current node_id
            }
            else if (leader == "data") {
                ss >> input_data;
                set_node_data(static_cast<T_data>(input_data), node_id);
            }
        }
        if (tmp_size != 0) {
            if (tmp_size != graph.size())
                cout << "Error: number of nodes in file " << graph.size() << " does not match size input " << tmp_size << endl; 
        }
    }
}; // end class Graph
// ##########################################################################
// #
// #
// #
// # end class Graph
// #
// #
// #    
// ##########################################################################


// ##########################################################################
// #                            class Hex
// #  creates a Graph object as a member of Hex using :either
// #      method load_board_from_file or make_board
// #  methods for defining and drawing the pictorial representation of the board
// #  methods for a person to play against this program
// #
// ##########################################################################
class Hex {
  public:
    Hex() = default;
    ~Hex() = default;
    // the actual board is created either by make_board(int size) or load_board_from_file(string filename)
    // this is better than 2 very large constructors that only differ by input type
    // because we clearly know what each does from its name

    enum class marker { empty = 0, playerX = 1, playerO = 2 }; // for the data held at each board position
    enum class Do_move { naive = 0, monte_carlo };

    // row and col on the hexboard to address a hexagon
    // row and col are seen by the human player so we use 1-based indexing
    // the linear_index conversion method handles this
    struct RowCol {
        int row;
        int col;

        RowCol(int row = 0, int col = 0) : row(row), col(col) {} // initialize to illegal position as sentinel
    };

    Timing winner_assess_time; // measure cumulative time for assessing the game
    Timing move_simulation_time; // measure cumulative time for simulating moves

    friend class Graph<marker>;
    Graph<marker> hex_graph;
        // a member of Hex that is a Graph object, using "composition" instead
        // of inheritance. The graph content is created by either make_board()
        // or load_board_from_file()
    
    int edge_len = 0;
    int max_idx = 0; // maximum linear index
    int move_count = 0; // number of moves played during the game: each player's move adds 1 (both moves = a ply)
    vector<int> rand_nodes; // use in rand move method

  private:
    vector<vector<int>> start_border; // holds indices to the top and left edges of the board
    vector<vector<int>> finish_border; // holds indices to the bottom and right edges of the board
    vector<vector<RowCol>> move_seq; // history of moves: use ONLY indices 1 and 2 for outer vector
    vector<marker> &positions = hex_graph.node_data; // positions of all markers on the board: alias to Graph member
        
    // used by monte_carlo_move: pre-allocated memory by method set_storage
    vector<int> empty_hex_pos; // empty positions that are available for candidate move and for simulated moves
    vector<int> random_pos; // copy of empty_hex_pos (except the candidate move) to be shuffled
    vector<float> win_pct_per_move;
    // used by find_ends: pre-allocated memory by method set_storage
    vector<int> neighbors;
    vector<int> captured;

  public:
    // ostream overloads
    // output a RowCol in an output stream
    friend ostream &operator<<(ostream &os, const RowCol &rc) 
    {
        os << "Row: " << rc.row << " Col: " << rc.col;
        return os;
    }

    friend ostream &operator<<(ostream &os, const Hex::marker &m)
    {
        if (m == Hex::marker::playerX)
            os << "player1";
        else if (m == Hex::marker::playerO)
            os << "player2";
        else
            os << "empty";

        return os;
    }

    friend fstream &operator<<(fstream &os, const Hex::marker &m)
    {
        if (m == Hex::marker::playerX)
            os << 1;
        else if (m == Hex::marker::playerO)
            os << 2;
        else
            os << 0;

        return os;
    }


  private:

    void set_storage(int max_idx)
    {
        empty_hex_pos.reserve(max_idx);
        random_pos.reserve(max_idx);
        win_pct_per_move.reserve(max_idx);
        captured.reserve(max_idx/2 + 1);  // for one player, can only be half the board positions + 1 for the side that goes first
        hex_graph.set_storage(max_idx); // using Graph method
        neighbors.reserve(6);
    }

    // METHODS FOR DRAWING THE BOARD
    // return hexboard marker based on value and add the spacer lines ___ needed to draw the board
    string symdash(marker val, bool last = false) const
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
        else if (val == marker::playerX)
            symunit = x + spacer;
        else if (val == marker::playerO)
            symunit = o + spacer;
        else {
            cout << "Error: invalid hexboard value: " << val << endl;
            exit(-1);
        }

        return symunit;
    }

    // how many spaces to indent each line of the hexboard?
    string lead_space(int row) const { return string_by_n(" ", row * 2); }

    // some string constants used to draw the board
    const string connector = R"( \ /)";
    const string last_connector = R"( \)";

    // create vectors containing start and finish borders for both sides
    void define_borders() // tested OK
    {
        int row;
        int col;

        // yes--we could this all in one loop but, this is much more obvious

        // initialize the inner vectors as empty. NOTE: the zero index for the outer
        // vector should NEVER be used
        for (int i = 0; i < 3; i++) {
            start_border.push_back(vector<int>{});
            finish_border.push_back(vector<int>{});
        }

        for (int row = 1, col = 1; col < edge_len + 1; col++) {
            start_border[enum2int(marker::playerX)].push_back(linear_index(row, col));
        }
        for (int row = edge_len, col = 1; col < edge_len + 1; col++) {
            finish_border[enum2int(marker::playerX)].push_back(linear_index(row, col));
        }

        for (int row = 1, col = 1; row < edge_len + 1; row++) {
            start_border[enum2int(marker::playerO)].push_back(linear_index(row, col));
        }

        for (int row = 1, col = edge_len; row < edge_len + 1; row++) {
            finish_border[enum2int(marker::playerO)].push_back(linear_index(row, col));
        }
    }

  public:
    // for random shuffling of board moves

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 rng{seed};

    template <typename T>  // cast enum class to int
    int enum2int(T t) { return static_cast<int>(t); }

    void make_board(int border_len=7) // initialize board positions
    {
        // initialize all members
        edge_len = border_len;
        max_idx = edge_len * edge_len; // same as size

        // REMINDER!!!: row and col indices are treated as 1-based!

        // reserve storage
        set_storage(max_idx);

        // define the board regions and move sequences
        define_borders();
        initialize_move_seq();

        // initialize positions: and also node_data because positions is a reference
        // to it
        hex_graph.initialize_data(marker::empty, max_idx);

        // add nodes:  the required hexagonal "tiles" on the board
        // initial values:  all tiles are empty = 0
        for (int i = 0; i < max_idx; i++) {
            hex_graph.add_edge(i);   // create an empty edge container at each node
            rand_nodes.push_back(i); // vector of nodes
        }

        // add graph edges for adjacent hexes based on the layout of a Hex game
        // 4 corners of the board                            tested OK
        // upper left
        hex_graph.add_edge(linear_index(1, 1), linear_index(2, 1));
        hex_graph.add_edge(linear_index(1, 1), linear_index(1, 2));
        // lower right
        hex_graph.add_edge(linear_index(edge_len, edge_len), linear_index(edge_len, (edge_len - 1)));
        hex_graph.add_edge(linear_index(edge_len, edge_len), linear_index((edge_len - 1), edge_len));
        // upper right
        hex_graph.add_edge(linear_index(1, edge_len), linear_index(1, (edge_len - 1)));
        hex_graph.add_edge(linear_index(1, edge_len), linear_index(2, edge_len));
        hex_graph.add_edge(linear_index(1, edge_len), linear_index(2, (edge_len - 1)));
        // lower left
        hex_graph.add_edge(linear_index(edge_len, 1), linear_index((edge_len - 1), 1));
        hex_graph.add_edge(linear_index(edge_len, 1), linear_index(edge_len, 2));
        hex_graph.add_edge(linear_index(edge_len, 1), linear_index((edge_len - 1), 2));

        // 4 borders (excluding corners)  4 edges per node.
        // north-south edges: constant row, vary col
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
        // east-west edges: constant col, vary row
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
    } // end of make_board

    // not used for the simulation but good for testing
    void load_board_from_file(string filename)
    {
        hex_graph.load_graph_from_file(filename);

        // initialize Hex class members
        max_idx = hex_graph.count_nodes();
        edge_len = sqrt(max_idx);

        if (edge_len * edge_len != max_idx) {
            cout << "Error: incorrect size for hexboard. Got size = " << max_idx << endl;
            cout << "Size must have an integer square root for number of tiles in "
                    "edge "
                    "of board.\n";
            exit(-1);
        }

        // define the board regions and move sequences
        define_borders();
        initialize_move_seq();

        // additional vectors pre-allocation
        // something for rand_nodes if we keep it TODO
        // reserve storage
        set_storage(max_idx);
    }

    // methods for playing game externally defined
    void initialize_move_seq()
    {
        // for move_seq for players 1 and 2
        for (int i = 0; i < 3; i++) {
            move_seq.push_back(vector<RowCol>{});
            if (i > 0)
                move_seq[i].reserve(max_idx / 2 + 1);
        }
    }

    // print the ascii board on screen
    void display_board() const
    {
        {
            bool last; // last board value in the row: true or false?

            // number legend across the top of the board
            cout << "  " << 1;
            for (int col = 2; col < edge_len + 1; col++) {
                if (col < 10) {
                    cout << "   ";
                    cout << col;
                }
                else {
                    cout << "  ";
                    cout << col;
                }
            }
            cout << endl;
            // format two lines for each row (except the last)
            for (int row = 1; row < edge_len + 1; row++) {
                if (row < 10) {
                    cout << lead_space(row-1);
                    cout << row;
                    cout << " ";
                }
                else {
                    cout << lead_space(row - 2) << " ";
                    cout << row;
                    cout << " ";
                }
                for (int col = 1; col < edge_len + 1; col++) {
                    last = col < edge_len ? false : true;
                    cout << symdash(get_hex_marker(row, col), last); // add each column value
                }

                cout << endl; // line break for row

                // connector lines to show edges between board positions
                if (row != edge_len) {
                    cout << lead_space(row); // leading spaces for connector line
                    cout << string_by_n(connector, (edge_len - 1)) << last_connector << endl;
                }
                else {
                    cout << "\n\n"; // last row: no connector slashes
                }
            }
        }
    }

    // ##########################################################################
    // #             Class Hex game playing methods
    // ##########################################################################

    // fill entire board with random markers for side 1 and 2
    // Note: not used for the monte carlo simulation because we never randomize the entire board
    void simulate_hexboard_positions()
    {
        vector<int> allidx(hex_graph.count_nodes());

        iota(allidx.begin(), allidx.end(), 0);
        shuffle(allidx.begin(), allidx.end(), rng);

        for (int i : allidx) {
            if (is_empty(i)) {
                if (i % 2 == 0)
                    set_hex_marker(marker::playerX, i);
                else
                    set_hex_marker(marker::playerO, i);
            }
        }
    }

    void simulate_hexboard_positions(vector<int> empty_hex_positions)
    {
        shuffle(empty_hex_positions.begin(), empty_hex_positions.end(), rng);
        for (int i = 0; i < empty_hex_positions.size(); i++) {
            if (i % 2 == 0)
                // CRUCIAL: we don't test the board position for even/odd
                // we test the index to the currently empty positions on the board
                set_hex_marker(marker::playerX, empty_hex_positions[i]);
            else
                set_hex_marker(marker::playerO, empty_hex_positions[i]);
        }
    }

    // the program makes a random move
    // Not used for the monte carlo simulation: used for testing computer moves early on
    RowCol random_move()
    {
        RowCol rc;
        int maybe; // candidate node to place a marker on

        shuffle(rand_nodes.begin(), rand_nodes.end(), rng);

        for (int i = 0; i < max_idx; i++) {
            maybe = rand_nodes[i];
            if (is_empty(maybe)) {
                rc = row_col_index(maybe);
                break;
            }
        }
        if (rc.row == 0 && rc.col == 0) { // never found an empty position->no possible move
            rc.row = -1;
            rc.col = -1; // no move
        }

        return rc;
    }

    // the program makes a naive move to extend its longest path
    // Not used for the monte carlo simulation
    RowCol naive_move(marker side)
    {
        RowCol rc;
        RowCol prev_move;
        int prev_move_linear;
        vector<int> neighbor_nodes;

        if (move_seq[enum2int(side)].empty()) {
            shuffle(start_border[static_cast<int>(side)].begin(),
                    start_border[static_cast<int>(side)].end(), rng);
            for (int maybe : start_border[static_cast<int>(side)]) {
                if (is_empty(maybe)) {
                    rc = row_col_index(maybe);
                    return rc;
                }
            }
        }
        else {
            prev_move = move_seq[enum2int(side)].back();
            prev_move_linear = linear_index(prev_move);

            neighbor_nodes = hex_graph.get_neighbor_nodes(prev_move_linear, marker::empty);

            if (neighbor_nodes.empty())
                return random_move();

            shuffle(neighbor_nodes.begin(), neighbor_nodes.end(), rng);
            shuffle(neighbor_nodes.begin(), neighbor_nodes.end(), rng);

            for (int node : neighbor_nodes) {
                rc = row_col_index(node);
                if (rc.col > prev_move.col)
                    return rc;
            }
            rc = row_col_index(neighbor_nodes.back());
        }

        return rc;
    }

    RowCol monte_carlo_move(marker side, int n_trials)  
    {
        move_simulation_time.start();

        // method uses class fields: clear them instead of creating new objects each time 
        empty_hex_pos.clear();
        random_pos.clear();
        win_pct_per_move.clear();

        int wins = 0;
        marker winning_side;
        int best_move = 0;
        char pause;

        // loop over positions on the board to find available moves = empty positions
        for (int i = 0; i < max_idx; ++i) {
            if (is_empty(i)) {
                empty_hex_pos.push_back(i); // indices of where the board is empty
            }
        }

        int move_num = 0; // the index of empty hex positions that will be assigned the move to evaluate
        
        // loop over the available move positions: make eval move, setup positions to randomize
        for (move_num = 0; move_num < empty_hex_pos.size(); move_num++) {

            // make the computer's move to be evaluated
            set_hex_marker(side, empty_hex_pos[move_num]);
            wins = 0;  // reset the win counter across the trials

            // only on the first move, copy all the empty_hex_pos except 0 to the vector to be shuffled
            if (move_num == 0 ) {  
                for (auto j = 0; j < move_num; j++)
                    random_pos.push_back(empty_hex_pos[j]);  // with memory reserved, this is about as fast as an update by array index
                for (auto j = move_num; j < empty_hex_pos.size()-1; j++)
                    random_pos.push_back(empty_hex_pos[j + 1]);
            }
            else {    // for the other moves, faster to simply change 2 values
                random_pos[move_num - 1] = empty_hex_pos[move_num - 1];
                random_pos[move_num]     = empty_hex_pos[move_num + 1];  // this skips empty_hex_pos[move_num]
            }

            for (int trial = 0; trial < n_trials; ++trial)
            {
                simulate_hexboard_positions(random_pos);

                winning_side = find_ends(side, true);

                wins += (winning_side == side ? 1 : 0);
            }

            // calculate and save computer win percentage for this move
            win_pct_per_move.push_back(static_cast<float>(wins) / n_trials);

            // reverse the trial move
            set_hex_marker(marker::empty, empty_hex_pos[move_num]); 
        }

        // find the maximum computer win percentage across all the candidate moves
        float maxpct = 0.0;
        best_move = empty_hex_pos[0];
        for (int i = 0; i < win_pct_per_move.size(); ++i) {  // linear search
            if (win_pct_per_move[i] > maxpct)
            {
                maxpct = win_pct_per_move[i]; 
                best_move = empty_hex_pos[i];
            }
        }
        
        // restore the board
        fill_board(empty_hex_pos, marker::empty);

        move_simulation_time.cum();

        return row_col_index(best_move);
    }

    RowCol computer_move(marker side, Do_move how, int n_trials)
    {
        RowCol rc;

        switch (how)
        {
            case Do_move::naive:
                rc = naive_move(side);
                break;
            case Do_move::monte_carlo:
                rc = monte_carlo_move(side, n_trials);
                break;
            }
            
        set_hex_marker(side, rc);
        move_seq[enum2int(side)].push_back(rc);

        move_count++;
        return rc;
    }

    template<typename T>   // should be int, float or string
    T safe_input(const string & msg)
    {
        T input;
        while (true) {
            cin >> input;
            if (cin.fail()) {
                cin.clear();
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                cout << msg;
                cin >> input;
            }
            if (!cin.fail())
                break;
        }
        return input;
    }

    RowCol move_input(const string &msg)
    {
        int row, col;
        while (true) {
            cin >> row >> col;
            if (cin.fail()) {
                cin.clear();
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                cout << msg;
                cin >> row >> col;
            }
            if (!cin.fail())
                break;
        }
        return RowCol{row, col};
    }

    RowCol person_move(marker side)
    {
        RowCol rc;
        int row;
        int col;
        int val;
        bool valid_move = false;

        while (!valid_move) {
            cout << "Enter a move in an empty position that contains '.'" << endl;
            cout << "Enter your move as the row number and the column number, separated by a space.\n";
            cout << "The computer prompts row col:  and you enter 3 5, followed by the enter key.";
            cout << "Enter -1 -1 to quit..." << endl;
            cout << "row col: ";

            rc = move_input("Please enter 2 integers: ");

            if (rc.row == -1 || rc.col == -1) {
                rc.row = -1;
                rc.col = -1;
                return rc;
            }

            if (row == -5) { // hidden command to write the current board positions
                // to a file
                // prepare output file
                string filename = "Board Graph.txt";
                ofstream outfile;
                outfile.open(filename,
                             ios::out); // open a file to perform write operation
                // using file object
                if (!(outfile.is_open())) {
                    cout << "Error opening file: " << filename << " Terminating.\n";
                    exit(-1);
                }
                hex_graph.display_graph(outfile, true);
                outfile.close();
            }

            // rc.row = row;
            // rc.col = col;
            valid_move = is_valid_move(rc);
        }

        set_hex_marker(side, rc);
        move_seq[enum2int(side)].push_back(rc);

        move_count++;
        return rc;
    }

    bool is_valid_move(RowCol rc) const
    {
        int row = rc.row;
        int col = rc.col;

        string bad_position = "Your move used an invalid row or column.\n\n";
        string not_empty = "Your move didn't choose an empty position.\n\n";
        string msg = "";

        bool valid_move = true;

        if (row > edge_len || row < 1) {
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

    // finds the ends of a path through the board for one side
    // Might not look it but this is a DFS, but it doesn't build a path. It only
    // finds the start point given a point in the finish border.  If the end point is
    // in the start border, we have a winner.  We don't care about the middle steps of the path.
    marker find_ends(marker side, bool whole_board=false) 
    {
        winner_assess_time.start();

        int front = 0;
        deque<int> possibles; // MUST BE A DEQUE! hold candidate sequences across the board

        // method uses class fields neighbors and captured: clear them each time instead of creating new objects
        neighbors.clear();
        captured.clear();

        // test for positions in the finish border, though start border would also work: assumption fewer markers at the finish
        for (auto hex : finish_border[enum2int(side)]) {  //look through the finish border
            if (get_hex_marker(hex) == side) // if there is a marker for this side, add it
            {
                possibles.push_back(hex);   // we'll try to trace a path extending from each of these nodes
                captured.push_back(hex);    // it should never be added again
            }
        }

        while (!possibles.empty()) {
            front = 0;         // we will work off the front using index rather than iterator

            while (true) // extend, branch, reject, find winner for this sequence
            {
                if (is_in_start(possibles[front], side)) { // if node in start border we have a winner
                    winner_assess_time.cum();
                    return side;
                }

                // find neighbors of the current node that match the current side and exclude already captured nodes
                neighbors = hex_graph.get_neighbor_nodes(possibles[front], side, captured); 
                
                if (neighbors.empty()) {    
                    if (!possibles.empty())  // always have to do this before pop because c++ will terminate if you pop from empty
                        possibles.pop_front(); // pop this node because it has no neighbors
                    break; // go back to the top whether empty or not:  outer while loop will test if empty
                }
                else {  // when we have one or more neighbors:
                    possibles[front] = neighbors[0];  // advance the endpoint to this neighbor, get rid of the previous possible
                    captured.push_back(neighbors[0]);

                    for (int i = 1; i < neighbors.size(); ++i) {  // if there is more than one neighbor..
                        possibles.push_back(neighbors[i]); // a new possible finishing end point
                        captured.push_back(neighbors[i]);
                    }
                }
            } // while(true)
        } // while (!working.empty())

        winner_assess_time.cum();
        // we've exhausted all the possibles w/o finding a complete branch
        if (whole_board) {   // the winner has to be the other side because the side we tested doesn't have a complete path
            return (side == marker::playerO ? marker::playerX : marker::playerO); // just reverse the side
        }
        else
            return marker::empty; // no winner--too early in the game--no path from start to finish for this side
    }

    marker who_won()  // we use this when we may not have a full board, so do need to evaluate both sides
    {
        marker winner = marker::empty;
        vector<marker> sides{marker::playerX, marker::playerO};

        for (auto side : sides) {
            winner = find_ends(side);
            if (winner != marker::empty) {
                break;
            }
        }

        return winner; 
    }

    void play_game(Do_move how, int n_trials=1000)
    {
        RowCol person_rc;   // person's move
        RowCol computer_rc; // computer's move
        bool valid_move;
        bool person_first = true;
        string answer;
        marker person_marker;
        marker computer_marker;
        marker winning_side;

        clear_screen();
        cout << "\n\n";

        // who goes first?  break when we have a valid answer
        while (true) {
            cout << string_by_n("\n", 15);
            cout << "*** Do you want to go first? (enter y or yes or n or no) ";
            answer = safe_input<string>("Enter y or yes or n or no: ");

            if (is_in(tolower(answer), "yes")) {
                person_first = true;
                person_marker = marker::playerX;
                computer_marker = marker::playerO;

                cout << "\nYou go first playing X markers.\n";
                cout << "Make a path from the top row to the bottom.\n";
                cout << "The computer goes second playing O markers.\n";
                cout << string_by_n("\n", 2);

                break;
            }
            else if (is_in(tolower(answer), "no")) {
                person_first = false;
                person_marker = marker::playerO;
                computer_marker = marker::playerX;

                cout << "\nThe computer goes first playing X markers.\n";
                cout << "You go second playing O markers.\n";
                cout << "Make a path from the first column to the last column.\n";
                cout << string_by_n("\n", 2);

                break;
            }
            else
                cout << "    Please enter [y]es or [n]o\n";
        }

        move_count = 0;

        while (true) // move loop
        {
            switch (person_marker) {

            // person goes first
            case marker::playerX :     
                display_board();

                person_rc = person_move(person_marker);
                if (person_rc.row == -1) {
                    cout << "Game over! Come back again...\n";
                    exit(0);
                }

                computer_rc = computer_move(computer_marker, how, n_trials);
                clear_screen();
                cout << "The computer moved at " << computer_rc << "\n";
                cout << "Your move at " << person_rc << " was valid.\n\n\n";

                break;

            // computer goes first
            case marker::playerO :      
                
                computer_rc = computer_move(computer_marker, how, n_trials);
                cout << "The computer moved at "<< computer_rc << "\n\n";

                display_board();

                person_rc = person_move(person_marker);
                if (person_rc.row == -1) {
                    cout << "Game over! Come back again...\n";
                    exit(0);
                }

                clear_screen();
                cout << "Your move at " << person_rc << " was valid.\n";

                break;
            case marker::empty:
                cout << "Error: Player marker for human player cannot be empty.\n";
                exit(-1);
            }

            // test for a winner
            if (move_count >= (edge_len+edge_len - 1)) {
                winning_side = who_won(); // result is marker::empty, marker::playerX, or marker::playerO

                if (enum2int(winning_side)) {   // e.g., wasn't marker::empty
                    cout << "We have a winner. "
                         << (winning_side == person_marker ? "You won. Congratulations!"
                                                             : " The computer beat you )-:")
                         << "\nGame over. Come back and play again!\n\n";
                    display_board();
                    break;
                }
            }

        }
        
    }

    // game play methods use row and col for board positions
    // row and col indices are 1-based for end users playing the game
    
    void set_hex_marker(marker val, RowCol rc) { hex_graph.set_node_data(val, linear_index(rc));}

    void set_hex_marker(marker val, int row, int col) { hex_graph.set_node_data(val, linear_index(row, col)); }

    void set_hex_marker(marker val, int linear) { hex_graph.set_node_data(val, linear); }

    marker get_hex_marker(RowCol rc) const {return hex_graph.get_node_data(linear_index(rc)); }

    marker get_hex_marker(int row, int col) const { return hex_graph.get_node_data(linear_index(row, col)); }

    marker get_hex_marker(int linear) const { return hex_graph.get_node_data(linear); }

    void fill_board(vector<int> indices, marker value)
    {
        for (const auto idx : indices) {
            positions[idx] = value;
        }
    }

    // convert row, col position to a linear index to an array or map
    // graph and minimum cost path use linear indices
    // linear indexes are 0-based to access c++ data structures
    int linear_index(RowCol rc) const
    {
        return linear_index(rc.row, rc.col);
    }

    // convert row, col position to a linear index to an array or map
    int linear_index(int row, int col) const
    {
        row -= 1; // convert from input 1-based indexing to zero-based indexing
        col -= 1;
        if (row < edge_len && col < edge_len) {
            return (row * edge_len) + col;
        }
        else {
            cout << "Error: row or col >= edge length\n";
            exit(-1);
        }
    }

    // convert linear_index to RowCol index
    RowCol row_col_index(int linear) const
    {
        if (linear < max_idx) {
            return RowCol((linear / edge_len) + 1, (linear % edge_len) + 1);
        }
        else {
            cout << "Error: row or col >= edge length\n";
            return RowCol(-1, -1);
        }
    }

    bool is_in_start(int idx, marker side) const
    {
        if (side == marker::playerX) {
            return idx < edge_len;
        }
        else if (side == marker::playerO) {
            return idx % edge_len == 0;
        }
        else
            cout << "Error: invalid side. Must be " << marker::playerX << " or " << marker::playerO << ".\n";
            exit(-1);
    }

    bool is_in_finish(int idx, marker side) const
    {
        if (side == marker::playerX) {
            return idx > max_idx - edge_len - 1;
        }
        else if (side == marker::playerO) {
            return idx % edge_len == edge_len - 1;
        }
        else {
            exit(-1);
        }
    }

    inline bool is_empty(RowCol rc) const { return is_empty(linear_index(rc)); }

    inline bool is_empty(int linear) const { return get_hex_marker(linear) == marker::empty; }
};
// ######################################################
//
//
//
//                end class hexboard
//
//
// 
// ######################################################


int main(int argc, char *argv[])
{
    int size = 5;
    int n_trials = 1000;
    if (argc == 2)
        size = atoi(argv[1]);
    else if (argc == 3) {
        size = atoi(argv[1]);
        n_trials = atoi(argv[2]);
    }

    Hex hb;
    hb.make_board(size);
    
    hb.play_game(Hex::Do_move::monte_carlo, n_trials);

    cout << "Assessing who won took " << hb.winner_assess_time.show() << " seconds.\n";
    cout << "Simulating and evaluating moves took "
        << hb.move_simulation_time.show() - hb.winner_assess_time.show() << " seconds.\n";

    return 0;
}

#ifndef HEX_H
#define HEX_H


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

#include "helpers.cpp"
#include "graph.cpp"

using namespace std;

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
        // RowCol() = default;
        // ~RowCol() = default;
    };

    Timing winner_assess_time; // measure cumulative time for assessing the game
    Timing move_simulation_time; // measure cumulative time for simulating moves

    
    template <typename T> // should be int, float or string
    T safe_input(const string &msg)
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

    friend class Graph<marker>;
    Graph<marker> hex_graph;
    // a member of Hex that is a Graph object, using "composition" instead
    // of inheritance. The graph content is created by either make_board()
    // or load_board_from_file()

    int edge_len;
    int max_idx; // maximum linear index
    int move_count; // number of moves played during the game: each player's move adds 1 (both moves = a ply)
    vector<int> rand_nodes; // use in rand move method

    inline bool is_empty(int linear) const { return get_hex_marker(linear) == marker::empty; }
    inline bool is_empty(RowCol rc) const { return is_empty(linear_index(rc)); }

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
    // for random shuffling of board moves

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 rng{seed};

    // convert row, col position to a linear index to an array or map
    // graph and minimum cost path use linear indices
    // linear indexes are 0-based to access c++ data structures
    int inline linear_index(RowCol rc) const { return linear_index(rc.row, rc.col); }

    // convert row, col position to a linear index to an array or map
    int inline linear_index(int row, int col) const
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
    RowCol inline row_col_index(int linear) const
    {
        if (linear < max_idx) {
            return RowCol((linear / edge_len) + 1, (linear % edge_len) + 1);
        }
        else {
            cout << "Error: row or col >= edge length\n";
            return RowCol(-1, -1);
        }
    }

    // ostream overloads
    // output a RowCol in an output stream
    friend std::ostream &operator<<(std::ostream &os, const Hex::RowCol &rc)
    {
        os << "Row: " << rc.row << " Col: " << rc.col;
        return os;
    }

    friend std::ostream &operator<<(std::ostream &os, const Hex::marker &m)
    {
        if (m == Hex::marker::playerX)
            os << "player1";
        else if (m == Hex::marker::playerO)
            os << "player2";
        else
            os << "empty";

        return os;
    }

    friend std::fstream &operator<<(std::fstream &os, const Hex::marker &m)
    {
        if (m == Hex::marker::playerX)
            os << 1;
        else if (m == Hex::marker::playerO)
            os << 2;
        else
            os << 0;

        return os;
    }

    template <typename T> // cast enum class to int
    int enum2int(T t)
    {
        return static_cast<int>(t);
    }

    // 
    // externally defined methods of class Hex to draw and manage board in hex_board.cpp
    // 
    void make_board(int border_len = 7); // initialize board positions


    // not used for the simulation but good for testing
    void load_board_from_file(string filename);

    // externally defined methods
    // methods for playing game externally defined
    void initialize_move_seq();


    // print the ascii board on screen
    void display_board() const;

    // return hexboard marker based on value and add the spacer lines ___ needed to draw the board
    string symdash(marker val, bool last = false) const;

    // how many spaces to indent each line of the hexboard?
    string lead_space(int row) const;

    // some string constants used to draw the board
    static const string connector;
    static const string last_connector;

    // create vectors containing start and finish borders for both sides
    void define_borders(); // tested OK

    // game play methods use row and col for board positions
    // row and col indices are 1-based for end users playing the game

    void set_hex_marker(marker val, RowCol rc) { hex_graph.set_node_data(val, linear_index(rc)); }

    void set_hex_marker(marker val, int row, int col) { hex_graph.set_node_data(val, linear_index(row, col)); }

    void set_hex_marker(marker val, int linear) { hex_graph.set_node_data(val, linear); }

    marker get_hex_marker(RowCol rc) const { return hex_graph.get_node_data(linear_index(rc)); }

    marker get_hex_marker(int row, int col) const { return hex_graph.get_node_data(linear_index(row, col)); }

    marker get_hex_marker(int linear) const { return hex_graph.get_node_data(linear); }

    void fill_board(vector<int> indices, marker value)
    {
        for (const auto idx : indices) {
            positions[idx] = value;
        }
    }


    // externally defined methods of class Hex in file game_play.cpp
    void simulate_hexboard_positions(vector<int> empty_hex_positions);
    void simulate_hexboard_positions();
    RowCol random_move();
    RowCol naive_move(marker side);
    RowCol monte_carlo_move(marker side, int n_trials);
    RowCol computer_move(marker side, Do_move how, int n_trials);
    RowCol move_input(const string &msg);
    RowCol person_move(marker side);
    bool is_valid_move(RowCol rc) const;
    marker find_ends(marker side, bool whole_board);
    marker who_won();
    void play_game(Do_move how, int n_trials=1000);
    bool inline is_in_start(int idx, marker side) const;
    bool inline is_in_finish(int idx, marker side) const;

  private:
    void set_storage(int max_idx)
    {
        empty_hex_pos.reserve(max_idx);
        random_pos.reserve(max_idx);
        win_pct_per_move.reserve(max_idx);
        captured.reserve(max_idx / 2 +
                         1); // for one player, can only be half the board positions + 1 for the side that goes first
        hex_graph.set_storage(max_idx); // using Graph method
        neighbors.reserve(6);
    }

    // METHODS FOR DRAWING THE BOARD

};


#endif

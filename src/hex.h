// ##########################################################################
// #             Definition/Declaration of Class Hex 
// ##########################################################################

#ifndef HEX_H
#define HEX_H


#include <deque> // sequence of nodes in a path between start and destination
#include <iostream>
#include <random>
#include <stdlib.h> // for atoi()
#include <string>
#include <unordered_map> // container for definition of Graph
#include <vector>

#include "graph.h"
#include "timing.h"
#include "helpers.h"

using namespace std;



// ##########################################################################
// #             Definition/Declaration of Class Hex
// ##########################################################################
class Hex {
public:


  // row and col on the hexboard to address a hexagon
  // row and col are seen by the human player so we use 1-based indexing
  // the rc2l conversion method handles this
    struct RowCol {
        int row;
        int col;

        RowCol(int row = 0, int col = 0)
            : row(row), col(col) {} // initialize to illegal position as sentinel
    };

    enum class Marker {
        empty = 0,
        playerX = 1,
        playerO = 2
    }; // for the data held at each board position

    struct Move {
        Marker player;
        int row;
        int col;

        Move(Marker player, int row, int col): player(player), row(row), col(col) {}
    };   // used only for move_history


    // constructor/destructor
    Hex(size_t size): edge_len(size) { // enforce input requirement and invariant
            if ((size < 0) || (size % 2 == 0)) {
                throw std::invalid_argument(
                    "Bad size input. Must be odd, positive integer.");
            }
            max_idx = edge_len * edge_len;
            empty_idxs.reserve(max_idx);
            throw_away.reserve(max_idx);
            shuffle_idxs.reserve(max_idx);
            move_history.reserve(max_idx);
            for (size_t i=0; i < max_idx; ++i) {
                empty_idxs.emplace_back(i);  // add all positions-> all start empty
            }
            hex_graph = Graph<Marker>(max_idx, Marker::empty); // initializes all board positions to empty
    }
    // Hex::make_board() greats the graph of the board and the ascii display of the board

    ~Hex() = default;

//
// members
//
public:
    Graph<Marker> hex_graph;
    // a member of Hex that is a Graph object, using "composition" instead
    // of inheritance. The graph content is created by make_board().

    // for random shuffling of board moves
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 rng{seed}; // difference didn't matter: std::mt19937 std::minstd_rand

    //   Timing winner_assess_time;   // measure cumulative time for assessing the game
    Timing move_simulation_time; // measure cumulative time for simulating moves

private:
    const int edge_len;
    int max_idx; // maximum linear index
    int move_count{0}; // number of moves played during the game: each player's move adds 1
    vector<vector<int>> start_border; // indices to the top and left edges of the board
    vector<vector<int>> finish_border; // indices to the bottom and right edges of the board
    vector<Marker> &positions = hex_graph.node_data; // positions ofMarkers on the board: alias to Graph
    vector<Move> move_history;

    // used by monte_carlo_move: pre-allocated memory by method set_storage
    vector<int> empty_idxs;   // empty positions for simulated moves
    vector<int> shuffle_idxs; // copy of empty_idxs (except the candidate move)
    vector<int> throw_away;   // the copy that gets shuffled
    vector<int> wins_per_move;
    // used by find_ends: pre-allocated memory by method set_storage
    vector<int> neighbors;
    vector<int> captured;

  //
  // methods
  //

    //
    // externally defined methods of class Hex to draw and manage board in
    // hex_board.cpp
    //
    public:
        void make_board();
        void display_board() const; // print the ascii board on screen
    private:
        string symdash(Marker val, bool last = false) const; // return hexboard Marker and add the spacer lines ___ needed to draw the board
        string lead_space(int row) const; // how many spaces to indent each line of the hexboard?
        void define_borders(); // create vectors containing start and finish
                               // borders for both sides

    // externally defined methods of class Hex in file game_play.cpp
    public:
        void play_game(int n_trials = 1000);
    private:
        void simulate_hexboard_positions(vector<int> &empties, Marker person_side, Marker computer_side);
        array<Marker, 2> who_goes_first();
        RowCol monte_carlo_move(Marker side, int n_trials, Marker person_side);
        void do_move(Marker side, RowCol rc);
        RowCol computer_move(Marker side, int n_trials, Marker other_side);
        RowCol move_input(const string &msg) const;
        RowCol person_move(Marker side);
        bool is_valid_move(RowCol rc) const;
        Marker find_ends(Marker side, bool whole_board);
        Marker who_won();
        bool inline is_in_start(int idx, Marker side) const;

    // setters and getters for the board
    private:
        void set_hex_Marker(Marker val, RowCol rc) { positions[rc2l(rc)] = val; }

        void set_hex_Marker(Marker val, int row, int col) { positions[rc2l(row, col)] = val; }

        void set_hex_Marker(Marker val, int linear) { positions[linear] = val; }
        
        Marker get_hex_Marker(RowCol rc) const { return positions[rc2l(rc)]; }

        Marker get_hex_Marker(int row, int col) const { return positions[rc2l(row, col)]; }

        Marker get_hex_Marker(int linear) const { return positions[linear]; }

    private:
        template <typename T> // should be int, float or string
            T safe_input(const string &msg) {
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

        void set_storage(int max_idx) { // optimization to reduce memory allocations for resizing containers
            shuffle_idxs.reserve(max_idx);
            wins_per_move.reserve(max_idx);
            captured.reserve(max_idx / 2 + 1);
            neighbors.reserve(6);
        }

public:
    inline bool is_empty(int linear) const {return get_hex_Marker(linear) == Marker::empty;}

    inline bool is_empty(RowCol rc) const { return is_empty(rc2l(rc)); }

    // indexing the board positions: game play methods use row and col for board positions
    // row and col indices are 1-based for end users playing the game
    // Class graph uses 0-based linear indices
    // linear indexes are 0-based to access c++ data structures

    // convert row, col position to a linear index to an array or vector
    inline int rc2l(RowCol rc) const { return rc2l(rc.row, rc.col); }

    inline int rc2l(int row, int col) const
    {
        row -= 1; // convert from input 1-based indexing to zero-based indexing
        col -= 1;
        if (row < edge_len && col < edge_len) {
            return (row * edge_len) + col;
        }
        else {
            throw invalid_argument("Bad row or col input: >= edge length\n");
        }
    }

    // convert linear index to RowCol index
    inline RowCol l2rc(int linear) const
    {
        if (linear < max_idx) {
            return RowCol((linear / edge_len) + 1, (linear % edge_len) + 1);
        }
        else {
            throw invalid_argument("Error: position index greater than number of positions on the board.\n");
        }
    }

    // ostream overloads
    friend std::ostream &operator<<(std::ostream &os, const Hex::RowCol &rc)
    {
        os << "Row: " << rc.row << " Col: " << rc.col;
        return os;
    }

    friend std::ostream &operator<<(std::ostream &os, const Hex::Marker &m)
    {
        if (m == Hex::Marker::playerX)
            os << "playerX";
        else if (m == Hex::Marker::playerO)
            os << "playerO";
        else
            os << "empty";
        return os;
    }

    friend std::fstream &operator<<(std::fstream &os, const Hex::Marker &m)
    {
        if (m == Hex::Marker::playerX)
            os << "playerX";
        else if (m == Hex::Marker::playerO)
            os << "playerO";
        else
            os << "empty";
        return os;
    }

    // output a Move in an output stream
    friend std::ostream &operator<<(ostream &out, const Move &mv) {
        out <<  mv.player << " row: " << mv.row
            << " col: " << mv.col << endl;
        return out;
    }

    template <typename T> // cast enum class to int; works for different enum classes
    int enum2int(T t) { return static_cast<int>(t); }

    void fill_board(vector<int> indices, Marker value)
    {
        for (const auto idx : indices) {
            positions[idx] = value;
        }
    }
};

#endif

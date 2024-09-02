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
  enum class Marker {
    empty = 0,
    playerX = 1,
    playerO = 2
  }; // for the data held at each board position

//   enum class Do_move { naive = 0, monte_carlo };

  // row and col on the hexboard to address a hexagon
  // row and col are seen by the human player so we use 1-based indexing
  // the linear_index conversion method handles this
  struct RowCol {
    int row;
    int col;

    RowCol(int row = 0, int col = 0)
        : row(row), col(col) {} // initialize to illegal position as sentinel
    };

    // constructor/destructor
    Hex(size_t size): edge_len(size) { // enforce input requirement and invariant
            if ((size < 0) || (size % 2 == 0)) {
                throw std::invalid_argument(
                    "Bad size input. Must be odd, positive integer.");
            }
            max_idx = edge_len * edge_len;
            empty_idxs.resize(max_idx);
            for (size_t i=0; i < empty_idxs.size(); ++i) {
                empty_idxs[i] = i;  // add all positions-> all start empty
            }
            Graph<Marker> hex_graph(max_idx, Hex::Marker::empty);  // initializes all board positions to empty
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
  std::mt19937 rng{seed};

  Timing winner_assess_time;   // measure cumulative time for assessing the game
  Timing move_simulation_time; // measure cumulative time for simulating moves

private:
  const int edge_len;
  int max_idx; // maximum linear index
  int move_count{0}; // number of moves played during the game: each player's move adds 1
  vector<vector<int>> start_border; // indices to the top and left edges of the board
  vector<vector<int>> finish_border; // indices to the bottom and right edges of the board
  vector<vector<RowCol>> move_seq; // history of moves: use ONLY indices 1 and 2 for outer vector
  vector<Marker> &positions = hex_graph.node_data; // positions ofMarkers on the board: alias to Graph

  // used by monte_carlo_move: pre-allocated memory by method set_storage
  vector<int> empty_idxs; // empty positions that are available for candidate
                             // move and for simulated moves
  vector<int> shuffle_idxs; // copy of empty_idxs (except the candidate move)
                          // to be shuffled
  vector<float> win_pct_per_move;
  // used by find_ends: pre-allocated memory by method set_storage
  vector<int> neighbors;
  vector<int> captured;



  //
  // methods
  //
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

  void set_storage(int max_idx) { // optimization to reduce memory allocations
                                  // for resizing containers
    // empty_idxs.reserve(max_idx);
    shuffle_idxs.reserve(max_idx);
    win_pct_per_move.reserve(max_idx);
    captured.reserve(max_idx / 2 + 1);
    // for a player, can only be half the board positions + 1 for the player
    // that goes first
    hex_graph.set_storage(max_idx); // using Graph method
    neighbors.reserve(6);
  }

public:
    inline bool is_empty(int linear) const {return get_hex_Marker(linear) == Marker::empty;}

    inline bool is_empty(RowCol rc) const { return is_empty(linear_index(rc)); }

    // indexing the board positions: game play methods use row and col for board
    // positions row and col indices are 1-based for end users playing the game
    // Class graph uses 0-based linear indices
    // linear indexes are 0-based to access c++ data structures

    // convert row, col position to a linear index to an array or map
    int linear_index(RowCol rc) const { return linear_index(rc.row, rc.col); }

    // convert row, col position to a linear index to an array or map
    int linear_index(int row, int col) const
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

    // convert linear_index to RowCol index
    RowCol linear2row_col(int linear) const
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
    friend std::ostream &operator<<(std::ostream &os, const Hex::RowCol &rc)
    {
        os << "Row: " << rc.row << " Col: " << rc.col;
        return os;
    }

    friend std::ostream &operator<<(std::ostream &os, const Hex::Marker &m)
    {
        if (m == Hex::Marker::playerX)
            os << "player1";
        else if (m == Hex::Marker::playerO)
            os << "player2";
        else
            os << "empty";

        return os;
    }

    friend std::fstream &operator<<(std::fstream &os, const Hex::Marker &m)
    {
        if (m == Hex::Marker::playerX)
            os << 1;
        else if (m == Hex::Marker::playerO)
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
// externally defined methods of class Hex to draw and manage board in
// hex_board.cpp
//
  public:
    void make_board();
    void display_board() const; // print the ascii board on screen
  private:                              
    string symdash(Marker val, bool last = false) const; // return hexboard Marker and add the spacer lines ___ needed to draw the board
    string lead_space(int row) const; // how many spaces to indent each line of the hexboard?
    // some string constants used to draw the board
    const string connector = R"( \ /)";
    const string last_connector = R"( \)";
    void define_borders(); // create vectors containing start and finish borders for both sides

    void set_hex_Marker(Marker val, RowCol rc) { hex_graph.set_node_data(val, linear_index(rc)); }
    void set_hex_Marker(Marker val, int row, int col) { hex_graph.set_node_data(val, linear_index(row, col)); }
    void set_hex_Marker(Marker val, int linear) { hex_graph.set_node_data(val, linear); }
    Marker get_hex_Marker(RowCol rc) const { return hex_graph.get_node_data(linear_index(rc)); }
    Marker get_hex_Marker(int row, int col) const { return hex_graph.get_node_data(linear_index(row, col)); }
    Marker get_hex_Marker(int linear) const { return hex_graph.get_node_data(linear); }

    void fill_board(vector<int> indices, Marker value)
    {
        for (const auto idx : indices) {
            positions[idx] = value;
        }
    }

    // externally defined methods of class Hex in file game_play.cpp
    public:
        void play_game(int n_trials = 1000);
    private:
        void initialize_move_seq();
        void simulate_hexboard_positions(vector<int> empty_idxsitions, Marker computer_side);
        // RowCol random_move();
        // RowCol naive_move(Marker side);
        RowCol monte_carlo_move(Marker side, int n_trials);
        void do_move(Marker side, RowCol rc);
        RowCol computer_move(Marker side, int n_trials);
        RowCol move_input(const string &msg) const;   
        RowCol person_move(Marker side);
        bool is_valid_move(RowCol rc) const;
        Marker find_ends(Marker side, bool whole_board);
        Marker who_won();
        bool inline is_in_start(int idx, Marker side) const;
};

#endif

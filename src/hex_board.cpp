// ##########################################################################
// #             Class Hex methods to create hexboard
// ##########################################################################

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


#include "hex.h"

using namespace std;


// return hexboard Marker based on value and add the spacer lines ___ needed to draw the board
string Hex::symdash(Marker val, bool last) const
{
    string symunit;
    string dot = "."; // for Markers encoded as 0 == empty
    string x = "X"; // for Markers encoded as 1
    string o = "O"; // for Markers encoded as 2
    string spacer = "___"; // part of drawing the board as ascii characters

    if (last)
        spacer = "";

    if (val == Marker::empty)
        symunit = dot + spacer;
    else if (val == Marker::playerX)
        symunit = x + spacer;
    else if (val == Marker::playerO)
        symunit = o + spacer;
    else {
        throw invalid_argument("Error: invalid hexboard value.");
    }

    return symunit;
}

// how many spaces to indent each line of the hexboard?
string Hex::lead_space(int row) const { return string_by_n(" ", row * 2); }

// create vectors containing start and finish borders for both sides
void Hex::define_borders() // tested OK
{
    // yes--we could this all in one loop but, this is much more obvious

    // initialize the inner vectors as empty. NOTE: the zero index for the outer
    // vector should NEVER be used
    for (int i = 0; i != 3; ++i) {
        start_border.push_back(vector<int>{});
        finish_border.push_back(vector<int>{});
    }

    // top border
    for (int row = 1, col = 1; col < edge_len + 1; col++) {
        start_border[enum2int(Marker::playerX)].push_back(linear_index(row, col));
    }
    // bottom border
    for (int row = edge_len, col = 1; col < edge_len + 1; col++) {
        finish_border[enum2int(Marker::playerX)].push_back(linear_index(row, col));
    }
    // left border
    for (int row = 1, col = 1; row < edge_len + 1; row++) {
        start_border[enum2int(Marker::playerO)].push_back(linear_index(row, col));
    }
    // right border
    for (int row = 1, col = edge_len; row != edge_len + 1; ++row) {
        finish_border[enum2int(Marker::playerO)].push_back(linear_index(row, col));
    }
}

void Hex::make_board() 
{
    // REMINDER!!!: row and col indices are treated as 1-based!

    // reserve storage
    set_storage(max_idx);

    // define the board regions and move sequences
    define_borders();
    initialize_move_seq();

    // add vector<Edge> for each node to hold edges in the graph for each node
    // initial values:  all tiles are empty = 0
    for (int i = 0; i != max_idx; ++i) {
        hex_graph.create_edge_container(i); // create an empty edge container at each node
        rand_nodes.push_back(i);            // vector of nodes
        // NOTE: node_data, which holds the Marker at each board position,
        //              initialized by class Graph constructor, called by class Hex constructor
        
    }

    // add graph edges for adjacent hexes based on the layout of a Hex game
    //    linear indices run from 0 at upper, left then across the row,
    //    then down 1 row at the left edge, and across, etc.
    // 
    // 4 corners of the board: 2 or 3 edges per node                            
    // upper left
    hex_graph.add_edge(linear_index(1, 1), linear_index(2, 1));
    hex_graph.add_edge(linear_index(1, 1), linear_index(1, 2));
    // upper right
    hex_graph.add_edge(linear_index(1, edge_len), linear_index(1, (edge_len - 1)));
    hex_graph.add_edge(linear_index(1, edge_len), linear_index(2, edge_len));
    hex_graph.add_edge(linear_index(1, edge_len), linear_index(2, (edge_len - 1)));
    // lower right
    hex_graph.add_edge(linear_index(edge_len, edge_len), linear_index(edge_len, (edge_len - 1)));
    hex_graph.add_edge(linear_index(edge_len, edge_len), linear_index((edge_len - 1), edge_len));
    // lower left
    hex_graph.add_edge(linear_index(edge_len, 1), linear_index((edge_len - 1), 1));
    hex_graph.add_edge(linear_index(edge_len, 1), linear_index(edge_len, 2));
    hex_graph.add_edge(linear_index(edge_len, 1), linear_index((edge_len - 1), 2));

    // 4 borders (excluding corners)  4 edges per node.
    // north-south edges: constant row, vary col
    for (int c = 2; c != edge_len; ++c) {
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
    for (int r = 2; r != edge_len; ++r) {
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
    for (int r = 2; r != edge_len; ++r) {
        for (int c = 2; c != edge_len; ++c) {
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c + 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c + 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c));
            hex_graph.add_edge(linear_index(r, c), linear_index(r + 1, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r, c - 1));
            hex_graph.add_edge(linear_index(r, c), linear_index(r - 1, c));
        }
    }
} // end of make_board

// methods for playing game externally defined
void Hex::initialize_move_seq()
{
    // for move_seq for players 1 and 2
    for (int i = 0; i != 3; ++i) {
        move_seq.push_back(vector<RowCol>{});
        if (i > 0)
            move_seq[i].reserve(max_idx / 2 + 1);
    }
}

// print the ascii board on screen
void Hex::display_board() const
{
    {
        bool last; // last board value in the row: true or false?

        // number legend across the top of the board
        cout << "  " << 1;
        for (int col = 2; col != edge_len + 1; ++col) {
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
        for (int row = 1; row != edge_len + 1; ++row) {
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
            for (int col = 1; col != edge_len + 1; ++col) {
                last = col < edge_len ? false : true;
                cout << symdash(get_hex_Marker(row, col), last); // add each column value
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

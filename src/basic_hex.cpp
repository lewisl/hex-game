// Draw a hexboard and play the game of hex
// Run as ./basic_hex 5 where the single argument is the number of hex's in a row/col
// For now, the hexboard class is trivial and incomplete.
// Programmer: Lewis Levin
// Date: April 2023

#include <iostream>
#include <stdlib.h>
#include <random>
#include <unordered_map>
#include <cmath>
#include <string>
#include <array>

using namespace std;

// some string constants used to draw the board
const string connector = R"( \ /)";
const string last_connector = R"( \)";

struct RankCol {
    int rank;
    int col;
};


// catenate multiple copies of a string
string string_by_n(string s, int n)
{
    string ret;
    for (int i=0; i<n; i++) {
        ret += s;
    }
    return ret;
}


void clear_screen()
{
    cout << u8"\033[2J";
}

// how many spaces to indent each line of the hexboard?
string lead_space(int rank)
{
    return string_by_n(" ", rank * 2);
}


// ######################################################
// class hexboard
// ######################################################
class HexBoard
{
private:
//  unordered_map<int, int> positions;
    vector<int> positions;
    int max_idx;
    
    int linear_index(RankCol rc)
    {
        return (rc.rank * edge_len) + rc.col;
    }
    
    int linear_index(int rank, int col)
    {
        return (rank * edge_len) + col;
    }
    
    // methods for drawing the board
    // return hexboard symbol based on value 
    //     and add the spacer lines ___ needed to draw the board
    string symdash(int val, bool last=false)
    {
        string symunit;
        string dot = ".";
        string x = "X";
        string o = "O";
        string spacer = "___";
        
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

    
public:    
    int edge_len;
    int max_rank;
    
    HexBoard(int edge_len) : edge_len(edge_len), positions(edge_len * edge_len, 0)
    {
        max_rank = edge_len - 1;
        max_idx = edge_len * edge_len;
    }
    
    void simulate_hexboard_positions()
    {                
        for (int i=0; i < positions.size(); i++)
            positions[i] = rand() % 3;
    }
    
    void set_hex_position(int val, RankCol rc)
    {
        positions[linear_index(rc)] = val;
    }
    
    void set_hex_position(int val, int rank, int col)
    {
        positions[linear_index(rank, col)] = val;
    }
    
    int get_hex_position(RankCol rc)
    {
        return positions[linear_index(rc)];
    }
    
    int get_hex_position(int rank, int col)
    {
        return positions[linear_index(rank, col)];
    }
    
    // create the visual board as a single ostream to use with cout << board
    friend ostream& operator<< (ostream& os, HexBoard board)
    {
        bool last;   // last board value in the rank: true or false?
        
        // format two lines for each rank (except the last)
        for (int rank=0; rank < board.edge_len; rank++) {  
            os << lead_space(rank);
            for (int col=0; col < board.edge_len; col++) {  
                last = col < board.max_rank ? false : true;
                os << board.symdash(board.get_hex_position(rank, col), last);  // add each column value
            }
            os << endl; // line break for rank
            if (rank != board.max_rank) {
                os << lead_space(rank);  // leading spaces for connector line
                os << string_by_n(connector, board.max_rank) << last_connector << endl;
            }
            else {
                os << "\n\n";      // last rank: no connector slashes
            }
        }
        return os;
    }
    
    
    bool is_valid_move(RankCol rc)
    {
        int rank = rc.rank;
        int col = rc.col;
        
        string bad_position = "Your move used an invalid rank or column.\n\n";
        string bad_value = "Your move didn't choose an empty position.\n\n";
        string msg = "";
        
        bool valid_move = true;
        
        if (rank > max_rank || rank < 0) {
            valid_move = false;
            msg = bad_position;
        }
        if (col > max_rank || col < 0) {
            valid_move = false;
            msg = bad_position;
        }
        if (get_hex_position(rc) != 0) {
            valid_move = false;
            msg = bad_value;
        }
        
        cout << msg;
        
        return valid_move;
    }
};
// ######################################################
// end class hexboard
// ######################################################




RankCol prompt_for_move(HexBoard board)
{
    RankCol rc;
    int rank;
    int col;
    int val;
    bool valid_move = false;
    
    while (!valid_move)
        {
            cout << "The program is playing X markers.  Your are playing O markers.\n";
            cout << "Enter a move in an empty position: one that contains '.'" << endl;
            cout << "(Note for Programmers: end-users use 1-indexing, so that's what we use...)\n";
            cout << "Enter -1 to quit..." << endl;
            cout << "Enter the rank (the row, using Chess terminology)... ";
            
            cin >> rank;
            
            if (rank == -1) {
                rc.rank=-1; rc.col = -1;
                return rc;
            }
            else {
                rc.rank = rank-1;             
            }
            
            cout << "Enter the column... ";
            
            cin >> col;
            
            if (col == -1) {
                rc.rank=-1; rc.col = -1;
                return rc;
            }
            else {
                rc.col = col-1;
            }
            
            valid_move = board.is_valid_move(rc);
        }

    return rc;
}

void play_game(HexBoard &hex)
{
    RankCol rc;
    bool valid_move;
    bool end_game = false;
    int player_marker = 2;
    
    hex.simulate_hexboard_positions();
    
    while (true)
    {
        cout << "\n\n";
        cout << hex << endl;
        
        valid_move = false;
        while (!valid_move) {
            rc = prompt_for_move(hex);
            if (rc.rank == -1) {
                cout << "Game over! Maybe next time...\n";
                end_game = true;
                break;
            }
            else {
                valid_move = hex.is_valid_move(rc);
            }
        }
        if (end_game) break;
        if (valid_move) {
            hex.set_hex_position(player_marker, rc);
            clear_screen();
            cout << "Good move!";
        }
    }
}



int main(int argc, char *argv[]) {
    srand(time(0));
    
    int size=5;
    HexBoard xb (size); 
    
    if (argc == 2)
        size = atoi(argv[1]);
    
    play_game(xb);
                
    return 0;
}
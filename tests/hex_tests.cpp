// Test functions for the hexboard and graph classes 

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
// test functions
// ##########################################################################

void test_board_graph(int edge_len) 
{
    HexBoard hb;
    hb.make_board(edge_len);
    hb.display_graph();
}


void test_board_regions(int edge_len)
{
    HexBoard hb;
    hb.make_board(edge_len);
    
}


void test_index_conversion(HexBoard & hb)
{
    int edge_len = hb.edge_len;
    
    RankCol rc_0 = hb.rank_col_index(0);
    RankCol rc_3 = hb.rank_col_index(3);
    RankCol rc_11 = hb.rank_col_index(11);
    RankCol rc_19 = hb.rank_col_index(19);
    RankCol rc_24 = hb.rank_col_index(24);
    
    cout << "\nTest rank_col_index\n";
    cout << "edge length is " << hb.edge_len << endl;
    cout << "linear input: " << setw(3) << 0 << " " << rc_0 << "\n";
    cout << "linear input: " << setw(3) << 3 << " " << rc_3 << "\n";
    cout << "linear input: " << setw(3) << 11 << " " << rc_11 << "\n";
    cout << "linear input: " << setw(3) << 19 << " " << rc_19 << "\n";
    cout << "linear input: " << setw(3) << 24 << " " << rc_24 << "\n";
    
    cout << "\nTest linear_index with RankCol input\n";
    cout << rc_0 << " linear " << setw(3) << hb.linear_index(rc_0) << "\n";
    cout << rc_3 << " linear " << setw(3) << hb.linear_index(rc_3) << "\n";
    cout << rc_11 << " linear " << setw(3) << hb.linear_index(rc_11) << "\n";
    cout << rc_19 << " linear " << setw(3) << hb.linear_index(rc_19) << "\n";
    cout << rc_24 << " linear " << setw(3) << hb.linear_index(rc_24) << "\n";
    
    cout << "\nTest linear_index with rank, col input\n";
    cout << rc_0 << " linear " << setw(3) << hb.linear_index(1,1) << "\n";
    cout << rc_3 << " linear " << setw(3) << hb.linear_index(1,4) << "\n";
    cout << rc_11 << " linear " << setw(3) << hb.linear_index(3,2) << "\n";
    cout << rc_19 << " linear " << setw(3) << hb.linear_index(4,5) << "\n";
    cout << rc_24 << " linear " << setw(3) << hb.linear_index(5,5) << "\n";
    
    
}

void test_index_conversion(int edge_len)
{
    HexBoard hb;
    hb.make_board(edge_len);
    
    RankCol rc_0 = hb.rank_col_index(0);
    RankCol rc_3 = hb.rank_col_index(3);
    RankCol rc_11 = hb.rank_col_index(11);
    RankCol rc_19 = hb.rank_col_index(19);
    RankCol rc_24 = hb.rank_col_index(24);
    
    cout << "\nTest rank_col_index\n";
    cout << "edge length is " << hb.edge_len << endl;
    cout << "linear input: " << setw(3) << 0 << " " << rc_0 << "\n";
    cout << "linear input: " << setw(3) << 3 << " " << rc_3 << "\n";
    cout << "linear input: " << setw(3) << 11 << " " << rc_11 << "\n";
    cout << "linear input: " << setw(3) << 19 << " " << rc_19 << "\n";
    cout << "linear input: " << setw(3) << 24 << " " << rc_24 << "\n";
    
    cout << "\nTest linear_index with RankCol input\n";
    cout << rc_0 << " linear " << setw(3) << hb.linear_index(rc_0) << "\n";
    cout << rc_3 << " linear " << setw(3) << hb.linear_index(rc_3) << "\n";
    cout << rc_11 << " linear " << setw(3) << hb.linear_index(rc_11) << "\n";
    cout << rc_19 << " linear " << setw(3) << hb.linear_index(rc_19) << "\n";
    cout << rc_24 << " linear " << setw(3) << hb.linear_index(rc_24) << "\n";
    
    cout << "\nTest linear_index with rank, col input\n";
    cout << rc_0 << " linear " << setw(3) << hb.linear_index(1,1) << "\n";
    cout << rc_3 << " linear " << setw(3) << hb.linear_index(1,4) << "\n";
    cout << rc_11 << " linear " << setw(3) << hb.linear_index(3,2) << "\n";
    cout << rc_19 << " linear " << setw(3) << hb.linear_index(4,5) << "\n";
    cout << rc_24 << " linear " << setw(3) << hb.linear_index(5,5) << "\n";

}

void test_get_and_set(int edge_len)
{
    HexBoard hb;
    hb.make_board(edge_len);
    RankCol rc = {3,3};
    hb.set_hex_position(3, rc);      // with struct index
    hb.set_hex_position(4, 4, 4);    // with explicit rank and col indices
    
    cout << "value is " << hb.get_hex_position(rc) << "\n";
    cout << "value is " << hb.get_hex_position(4,4) << "\n";
    cout << "value is " << hb.get_hex_position(hb.linear_index(rc.rank, rc.col)) << "\n";
    
}

// ##########################################################################
// end test functions
// ##########################################################################


/*
    start playing the game
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

#include "hex.h"
#include "helpers.h"

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

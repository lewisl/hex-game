/*
    start playing the game:  this is the "main" for running the game


    Run as hex [size] [n_trials]
*/

#include "hex.h"

int main(int argc, char *argv[])
{
    int size = 5;
    int n_trials = 1000;
    if (argc == 1)
        ;  // run with defaults
    else if (argc == 2)
        size = atoi(argv[1]);
    else if (argc == 3) {
        size = atoi(argv[1]);
        n_trials = atoi(argv[2]);}
    else {
        cout << "Wrong number of input arguments:\n"
            << "Run as hex [size] [n_trials]. exiting..." << endl;
        return 0;}

    if ((size < 0) || (size % 2 == 0)) {
        throw std::invalid_argument(
            "Bad size input. Must be odd, positive integer.");
    }

    Hex hb(size);  // create the game object
    hb.make_board();
    
    hb.play_game(Hex::Do_move::monte_carlo, n_trials);

    cout << "Assessing who won took " << hb.winner_assess_time.show() << " seconds.\n";
    cout << "Simulating and evaluating moves took "
        << hb.move_simulation_time.show() - hb.winner_assess_time.show() << " seconds.\n";

    return 0;
}

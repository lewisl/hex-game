/*
    start playing the game:  this is the "main" for running the game


    Run as hex [size] [n_trials]
*/

#include "hex.h"

int main(int argc, char *argv[])
{
    int size = 5;
    int n_trials = 1000;

    try {
        if (argc == 1) {
            ;  // run with defaults
        }
        else if (argc == 2) {
            size = std::stoi(argv[1]);
        }
        else if (argc == 3) {
            size = std::stoi(argv[1]);
            n_trials = std::stoi(argv[2]);
        }
        else {
            cerr << "Wrong number of input arguments:\n"
                << "Run as hex [size] [n_trials]. exiting..." << endl;
            return 1;
        }
    }
    catch (const std::invalid_argument& e) {
        cerr << "Error: Invalid argument. Please provide valid integers for size and n_trials.\n";
        return 1;
    }
    catch (const std::out_of_range& e) {
        cerr << "Error: Argument out of range. Please provide smaller values.\n";
        return 1;
    }

    if (size <= 0) {
        cerr << "Error: Bad size input. Must be a positive integer.\n";
        return 1;
    }

    Hex hb(size);  // create the game object
    hb.make_board();

    hb.play_game(n_trials);

    // cout << "Assessing who won took " << hb.winner_assess_time.show() << " seconds.\n";
    cout << "Simulating and evaluating moves took "
        << hb.move_simulation_time.show() << " seconds.\n";

    return 0;
}

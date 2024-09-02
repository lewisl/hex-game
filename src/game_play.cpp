// ##########################################################################
// #             Class Hex game playing methods
// ##########################################################################


#include "hex.h"
#include "helpers.h"
#include "timing.h"
#include <stdexcept>
#include <system_error>

using namespace std;


void Hex::simulate_hexboard_positions(vector<int> empties, Marker computer_side)  // argument is a copy!
{
    shuffle(empties.begin(), empties.end(), rng);  

    // swap the scalars each iteration to alternate markers
    Marker current = computer_side == Marker::playerX ? Marker::playerO : Marker ::playerX;
    Marker next = computer_side;
    Marker tmp = Marker::empty;
    for (int i = 0; i != empties.size(); ++i) {
        set_hex_Marker(current, empties[i]);
        tmp = current; current = next; next = tmp;  // faster than std::swap
    }
}


Hex::RowCol Hex::monte_carlo_move(Marker side, int n_trials)
{

    // method uses class fields: clear them instead of creating new objects each time
    shuffle_idxs.clear();
    win_pct_per_move.clear();

    int wins = 0;
    Marker winning_side;
    int best_move = 0;

    int move_num = 0; // the index of empty hex positions that will be assigned the move to evaluate

    // loop over the available move positions: make eval move, setup positions to randomize
    for (move_num = 0; move_num != empty_idxs.size(); ++move_num) {

        // make the computer's move to be evaluated
        set_hex_Marker(side, empty_idxs[move_num]);
        wins = 0; // reset the win counter across the trials

        // only on the first move, copy all the empty_idxs except 0 to the vector to be shuffled
        if (move_num == 0) {
            for (auto j = move_num; j != empty_idxs.size() - 1; ++j)  //every index but 0 opf empty_idxs copied into shuffle_idxs
                shuffle_idxs.push_back(empty_idxs[j + 1]);
        } else if (shuffle_idxs.size() < move_num) { // faster to simply change 2 values
            shuffle_idxs[move_num - 1] = empty_idxs[move_num - 1]; // skip empty_idxs[move_num]
            shuffle_idxs[move_num] = empty_idxs[move_num + 1];
        } 
        else {
            shuffle_idxs[move_num - 1] = empty_idxs[move_num - 1];  // skip max index value of empty_idxs
        }

        for (int trial = 0; trial != n_trials; ++trial) {
            simulate_hexboard_positions(shuffle_idxs, side);

            winning_side = find_ends(side, true);

            wins += (winning_side == side ? 1 : 0);
        }

        // calculate and save computer win percentage for this move
        win_pct_per_move.push_back(static_cast<float>(wins) / n_trials);

        // reverse the trial move
        set_hex_Marker(Marker::empty, empty_idxs[move_num]);
    }

    // find the maximum computer win percentage across all the candidate moves
    float maxpct = 0.0;
    best_move = empty_idxs[0];
    for (int i = 0; i != win_pct_per_move.size(); ++i) { // linear search
        if (win_pct_per_move[i] > maxpct) {
            maxpct = win_pct_per_move[i];
            best_move = empty_idxs[i];
        }
    }

    // restore the board
    fill_board(empty_idxs, Marker::empty);

    return linear2row_col(best_move);
}

void Hex::do_move(Marker side, RowCol rc)
{
    set_hex_Marker(side, rc);

    // remove empty
    auto emptypos = find(empty_idxs.begin(), empty_idxs.end(), linear_index(rc));
    auto foo = empty_idxs.erase(emptypos);  // we don't use foo but we have to catch the return value
    
    move_count++;
}

Hex::RowCol Hex::computer_move(Marker side, int n_trials)
{
    RowCol rc;
    
    move_simulation_time.start();
    rc = monte_carlo_move(side, n_trials);
    move_simulation_time.cum();

    // set_hex_Marker(side, rc);
    do_move(side, rc);
    move_seq[enum2int(side)].push_back(rc);

    // move_count++;
    return rc;
}


Hex::RowCol Hex::move_input(const string &msg) const
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

    return Hex::RowCol{row, col};
}

Hex::RowCol Hex::person_move(Marker side)
{
    RowCol rc;
    bool valid_move = false;

    while (!valid_move) {
        cout << "Enter a move in an empty position that contains '.'" << endl;
        cout << "Enter your move as the row number and the column number, separated by a space.\n";
        cout << "The computer prompts row col:  and you enter 3 5, followed by the enter key.";
        cout << "Enter -1 -1 to quit..." << endl;
        cout << "row col: ";

        rc = move_input("Please enter 2 integers: ");

        if (rc.row == -1) {
            rc.col = -1;
            return rc;
        }

        if (rc.col == -1) {
            rc.row = -1;
            return rc;
        }

        if (rc.row == -5) { // hidden command to write the current board positions
            // to a file
            // prepare output file
            string filename = "Board Graph.txt";
            ofstream outfile;
            outfile.open(filename,
                            ios::out); // open a file to perform write operation
            // using file object
            if (!(outfile.is_open())) {
                throw invalid_argument("Error opening file.");
            }
            hex_graph.display_graph(outfile, true);
            outfile.close();
        }

        valid_move = is_valid_move(rc);
        }

        // set_hex_Marker(side, rc);
    // cout << " the move was " << rc << endl;
    do_move(side, rc);
    move_seq[enum2int(side)].push_back(rc);

    // move_count++;

    return rc;
}

bool Hex::is_valid_move(Hex::RowCol rc) const
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
    else if (get_hex_Marker(rc) != Hex::Marker::empty) {
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
Hex::Marker Hex::find_ends(Hex::Marker side, bool whole_board = false)
{
    int front = 0;
    deque<int> possibles; // MUST BE A DEQUE! hold candidate sequences across the board

    // method uses class fields neighbors and captured: clear them each time instead of creating new objects
    neighbors.clear();
    captured.clear();

    // test for positions in the finish border, though start border would also work: assumption fewer Markers at the finish
    for (auto hex : finish_border[enum2int(side)]) { //look through the finish border
        if (get_hex_Marker(hex) == side) // if there is a Marker for this side, add it
        {
            possibles.push_back(hex); // we'll try to trace a path extending from each of these nodes
            captured.push_back(hex); // it should never be added again
        }
    }

    while (!possibles.empty()) {
        front = 0; // we will work off the front using index rather than iterator

        while (true) // extend, branch, reject, find winner for this sequence
        {
            if (is_in_start(possibles[front], side)) { // if node in start border we have a winner
                return side;
            }

            // find neighbors of the current node that match the current side and exclude already captured nodes
            neighbors = hex_graph.get_neighbor_nodes(possibles[front], side, captured);

            if (neighbors.empty()) {
                if (!possibles.empty()) // always have to do this before pop because c++ will terminate if you pop from empty
                    possibles.pop_front(); // pop this node because it has no neighbors
                break; // go back to the top whether empty or not:  outer while loop will test if empty
            }
            else { // when we have one or more neighbors:
                possibles[front] = neighbors[0]; // advance the endpoint to this neighbor, get rid of the previous possible
                captured.push_back(neighbors[0]);

                for (int i = 1; i != neighbors.size(); ++i) { // if there is more than one neighbor..
                    possibles.push_back(neighbors[i]); // a new possible finishing end point
                    captured.push_back(neighbors[i]);
                }
            }
        } // while(true)
    } // while (!working.empty())

    // we've exhausted all the possibles w/o finding a complete branch
    if (whole_board) { // the winner has to be the other side because the side we tested doesn't have a complete path
        return (side == Marker::playerO ? Marker::playerX : Marker::playerO); // just reverse the side
    }
    else
        return Marker::empty; // no winner--too early in the game--no path from start to finish for this side
}

bool inline Hex::is_in_start(int idx, Marker side) const {
    if (side == Marker::playerX) {
        return idx < edge_len;
    } else if (side == Marker::playerO) {
        return idx % edge_len == 0;
    } else
        throw invalid_argument("Error: invalid side. Must be Marker::playerX  "
                            "or  Marker::playerO.\n");
}

// used when board may not be full, so do need to evaluate both sides
Hex::Marker Hex::who_won() 
{
    winner_assess_time.start();

    Marker winner = Marker::empty;
    vector<Marker> sides{Marker::playerX, Marker::playerO};

    for (auto side : sides) {
        winner = find_ends(side);
        if (winner != Marker::empty) {
            break;
        }
    }
    winner_assess_time.cum();

    return winner;
}

void Hex::play_game(int n_trials) 
{
    RowCol person_rc; // person's move
    RowCol computer_rc; // computer's move
    string answer;
    Marker person_Marker;
    Marker computer_Marker;
    Marker winning_side;

    clear_screen();
    cout << "\n\n";

    // who goes first?  break when we have a valid answer
    while (true) {
        cout << string_by_n("\n", 15);
        cout << "*** Do you want to go first? (enter y or yes or n or no) ";
        answer = safe_input<string>("Enter y or yes or n or no: ");

        if (is_in(tolower(answer), "yes")) {
            // person_first = true;
            person_Marker = Marker::playerX;
            computer_Marker = Marker::playerO;

            cout << "\nYou go first playing X Markers.\n";
            cout << "Make a path from the top row to the bottom.\n";
            cout << "The computer goes second playing O Markers.\n";
            cout << string_by_n("\n", 2);

            break;
        }
        else if (is_in(tolower(answer), "no")) {
            // person_first = false;
            person_Marker = Marker::playerO;
            computer_Marker = Marker::playerX;

            cout << "\nThe computer goes first playing X Markers.\n";
            cout << "You go second playing O Markers.\n";
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
        switch (person_Marker) {

        // person goes first
        case Marker::playerX:
            display_board();

            person_rc = person_move(person_Marker);

            if (person_rc.row == -1) {
                cout << "Game over! Come back again...\n";
                exit(0);
            }

            computer_rc = computer_move(computer_Marker, n_trials);
            clear_screen();
            cout << "The computer moved at " << computer_rc << "\n";
            cout << "Your move at " << person_rc << " was valid.\n\n\n";

            break;

        // computer goes first
        case Marker::playerO:

            computer_rc = computer_move(computer_Marker, n_trials);
            cout << "The computer moved at " << computer_rc << "\n\n";

            display_board();

            person_rc = person_move(person_Marker);
            if (person_rc.row == -1) {
                cout << "Game over! Come back again...\n";
                exit(0);
            }

            clear_screen();
            cout << "Your move at " << person_rc << " was valid.\n";

            break;
        case Marker::empty:
            throw invalid_argument("Error: Player Marker for human player cannot be empty.\n");
        }

        // test for a winner
        if (move_count >= (edge_len + edge_len - 1)) {
            winning_side = who_won(); // result is Marker::empty, Marker::playerX, or Marker::playerO

            if (enum2int(winning_side)) { // e.g., wasn't Marker::empty
                cout    << "We have a winner. "
                        << (winning_side == person_Marker ? "You won. Congratulations!" : " The computer beat you )-:")
                        << "\nGame over. Come back and play again!\n\n";
                display_board();
                break;
            }
        }
    }
}

// ##########################################################################
// #             Class Hex game playing methods
// ##########################################################################


#include "hex.h"
#include "helpers.h"
#include "timing.h"

using namespace std;


// fill entire board with random markers for side 1 and 2
// Note: not used for the monte carlo simulation because we never randomize the entire board
void Hex::simulate_hexboard_positions()
{
    vector<int> allidx(hex_graph.count_nodes());

    iota(allidx.begin(), allidx.end(), 0);
    shuffle(allidx.begin(), allidx.end(), rng);

    for (int i : allidx) {
        if (is_empty(i)) {
            if (i % 2 == 0)
                set_hex_marker(marker::playerX, i);
            else
                set_hex_marker(marker::playerO, i);
        }
    }
}

void Hex::simulate_hexboard_positions(vector<int> empty_hex_positions)
{
    shuffle(empty_hex_positions.begin(), empty_hex_positions.end(), rng);
    for (int i = 0; i != empty_hex_positions.size(); ++i) {
        if (i % 2 == 0)
            // CRUCIAL: we don't test the board position for even/odd
            // we test the index to the currently empty positions on the board
            set_hex_marker(marker::playerX, empty_hex_positions[i]);
        else
            set_hex_marker(marker::playerO, empty_hex_positions[i]);
    }
}

// the program makes a random move
// Not used for the monte carlo simulation: used for testing computer moves early on
Hex::RowCol Hex::random_move()
{
    RowCol rc;
    int maybe; // candidate node to place a marker on

    shuffle(rand_nodes.begin(), rand_nodes.end(), rng);

    for (int i = 0; i != max_idx; ++i) {
        maybe = rand_nodes[i];
        if (is_empty(maybe)) {
            rc = row_col_index(maybe);
            break;
        }
    }
    if (rc.row == 0 && rc.col == 0) { // never found an empty position->no possible move
        rc.row = -1;
        rc.col = -1; // no move
    }

    return rc;
}

// the program makes a naive move to extend its longest path
// Not used for the monte carlo simulation
Hex::RowCol Hex::naive_move(marker side)
{
    RowCol rc;
    RowCol prev_move;
    int prev_move_linear;
    vector<int> neighbor_nodes;

    if (move_seq[enum2int(side)].empty()) {
        shuffle(start_border[static_cast<int>(side)].begin(), start_border[static_cast<int>(side)].end(), rng);
        for (int maybe : start_border[static_cast<int>(side)]) {
            if (is_empty(maybe)) {
                rc = row_col_index(maybe);
                return rc;
            }
        }
    }
    else {
        prev_move = move_seq[enum2int(side)].back();
        prev_move_linear = linear_index(prev_move);

        neighbor_nodes = hex_graph.get_neighbor_nodes(prev_move_linear, marker::empty);

        if (neighbor_nodes.empty())
            return random_move();

        shuffle(neighbor_nodes.begin(), neighbor_nodes.end(), rng);
        shuffle(neighbor_nodes.begin(), neighbor_nodes.end(), rng);

        for (int node : neighbor_nodes) {
            rc = row_col_index(node);
            if (rc.col > prev_move.col)
                return rc;
        }
        rc = row_col_index(neighbor_nodes.back());
    }

    return rc;
}

Hex::RowCol Hex::monte_carlo_move(marker side, int n_trials)
{
    move_simulation_time.start();

    // method uses class fields: clear them instead of creating new objects each time
    empty_hex_pos.clear();
    random_pos.clear();
    win_pct_per_move.clear();

    int wins = 0;
    marker winning_side;
    int best_move = 0;
    // char pause;

    // loop over positions on the board to find available moves = empty positions
    for (int i = 0; i != max_idx; ++i) {
        if (is_empty(i)) {
            empty_hex_pos.push_back(i); // indices of where the board is empty
        }
    }

    int move_num = 0; // the index of empty hex positions that will be assigned the move to evaluate

    // loop over the available move positions: make eval move, setup positions to randomize
    for (move_num = 0; move_num != empty_hex_pos.size(); ++move_num) {

        // make the computer's move to be evaluated
        set_hex_marker(side, empty_hex_pos[move_num]);
        wins = 0; // reset the win counter across the trials

        // only on the first move, copy all the empty_hex_pos except 0 to the vector to be shuffled
        if (move_num == 0) {
            for (auto j = 0; j != move_num; ++j)
                random_pos.push_back(
                    empty_hex_pos[j]); // with memory reserved, this is about as fast as an update by array index
            for (auto j = move_num; j != empty_hex_pos.size() - 1; ++j)
                random_pos.push_back(empty_hex_pos[j + 1]);
        }
        else { // for the other moves, faster to simply change 2 values
            random_pos[move_num - 1] = empty_hex_pos[move_num - 1];
            random_pos[move_num] = empty_hex_pos[move_num + 1]; // this skips empty_hex_pos[move_num]
        }

        for (int trial = 0; trial != n_trials; ++trial) {
            simulate_hexboard_positions(random_pos);

            winning_side = find_ends(side, true);

            wins += (winning_side == side ? 1 : 0);
        }

        // calculate and save computer win percentage for this move
        win_pct_per_move.push_back(static_cast<float>(wins) / n_trials);

        // reverse the trial move
        set_hex_marker(marker::empty, empty_hex_pos[move_num]);
    }

    // find the maximum computer win percentage across all the candidate moves
    float maxpct = 0.0;
    best_move = empty_hex_pos[0];
    for (int i = 0; i != win_pct_per_move.size(); ++i) { // linear search
        if (win_pct_per_move[i] > maxpct) {
            maxpct = win_pct_per_move[i];
            best_move = empty_hex_pos[i];
        }
    }

    // restore the board
    fill_board(empty_hex_pos, marker::empty);

    move_simulation_time.cum();

    return row_col_index(best_move);
}

Hex::RowCol Hex::computer_move(marker side, Hex::Do_move how, int n_trials)
{
    RowCol rc;

    switch (how) {
    case Hex::Do_move::naive:
        rc = naive_move(side);
        break;
    case Hex::Do_move::monte_carlo:
        rc = monte_carlo_move(side, n_trials);
        break;
    }

    set_hex_marker(side, rc);
    move_seq[enum2int(side)].push_back(rc);

    move_count++;
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

Hex::RowCol Hex::person_move(marker side)
{
    RowCol rc;
    // int row;
    // int col;
    // int val;
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
                    cout << "Error opening file: " << filename << " Terminating.\n";
                    exit(-1);
                }
                hex_graph.display_graph(outfile, true);
                outfile.close();
            }

            valid_move = is_valid_move(rc);
        }

    set_hex_marker(side, rc);
    move_seq[enum2int(side)].push_back(rc);

    move_count++;

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
    else if (get_hex_marker(rc) != Hex::marker::empty) {
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
Hex::marker Hex::find_ends(Hex::marker side, bool whole_board = false)
{
    winner_assess_time.start();

    int front = 0;
    deque<int> possibles; // MUST BE A DEQUE! hold candidate sequences across the board

    // method uses class fields neighbors and captured: clear them each time instead of creating new objects
    neighbors.clear();
    captured.clear();

    // test for positions in the finish border, though start border would also work: assumption fewer markers at the finish
    for (auto hex : finish_border[enum2int(side)]) { //look through the finish border
        if (get_hex_marker(hex) == side) // if there is a marker for this side, add it
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
                winner_assess_time.cum();
                return side;
            }

            // find neighbors of the current node that match the current side and exclude already captured nodes
            neighbors = hex_graph.get_neighbor_nodes(possibles[front], side, captured);

            if (neighbors.empty()) {
                if (!possibles
                         .empty()) // always have to do this before pop because c++ will terminate if you pop from empty
                    possibles.pop_front(); // pop this node because it has no neighbors
                break; // go back to the top whether empty or not:  outer while loop will test if empty
            }
            else { // when we have one or more neighbors:
                possibles[front] =
                    neighbors[0]; // advance the endpoint to this neighbor, get rid of the previous possible
                captured.push_back(neighbors[0]);

                for (int i = 1; i != neighbors.size(); ++i) { // if there is more than one neighbor..
                    possibles.push_back(neighbors[i]); // a new possible finishing end point
                    captured.push_back(neighbors[i]);
                }
            }
        } // while(true)
    } // while (!working.empty())

    winner_assess_time.cum();
    // we've exhausted all the possibles w/o finding a complete branch
    if (whole_board) { // the winner has to be the other side because the side we tested doesn't have a complete path
        return (side == marker::playerO ? marker::playerX : marker::playerO); // just reverse the side
    }
    else
        return marker::empty; // no winner--too early in the game--no path from start to finish for this side
}

Hex::marker Hex::who_won() // we use this when we may not have a full board, so do need to evaluate both sides
{
    marker winner = marker::empty;
    vector<marker> sides{marker::playerX, marker::playerO};

    for (auto side : sides) {
        winner = find_ends(side);
        if (winner != marker::empty) {
            break;
        }
    }

    return winner;
}

void Hex::play_game(Hex::Do_move how, int n_trials) 
{
    RowCol person_rc; // person's move
    RowCol computer_rc; // computer's move
    // bool valid_move;
    // bool person_first = true;
    string answer;
    marker person_marker;
    marker computer_marker;
    marker winning_side;

    clear_screen();
    cout << "\n\n";

    // who goes first?  break when we have a valid answer
    while (true) {
        cout << string_by_n("\n", 15);
        cout << "*** Do you want to go first? (enter y or yes or n or no) ";
        answer = safe_input<string>("Enter y or yes or n or no: ");

        if (is_in(tolower(answer), "yes")) {
            // person_first = true;
            person_marker = marker::playerX;
            computer_marker = marker::playerO;

            cout << "\nYou go first playing X markers.\n";
            cout << "Make a path from the top row to the bottom.\n";
            cout << "The computer goes second playing O markers.\n";
            cout << string_by_n("\n", 2);

            break;
        }
        else if (is_in(tolower(answer), "no")) {
            // person_first = false;
            person_marker = marker::playerO;
            computer_marker = marker::playerX;

            cout << "\nThe computer goes first playing X markers.\n";
            cout << "You go second playing O markers.\n";
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
        switch (person_marker) {

        // person goes first
        case marker::playerX:
            display_board();

            person_rc = person_move(person_marker);


            if (person_rc.row == -1) {
                cout << "Game over! Come back again...\n";
                exit(0);
            }

            computer_rc = computer_move(computer_marker, how, n_trials);
            clear_screen();
            cout << "The computer moved at " << computer_rc << "\n";
            cout << "Your move at " << person_rc << " was valid.\n\n\n";

            break;

        // computer goes first
        case marker::playerO:

            computer_rc = computer_move(computer_marker, how, n_trials);
            cout << "The computer moved at " << computer_rc << "\n\n";

            display_board();

            person_rc = person_move(person_marker);
            if (person_rc.row == -1) {
                cout << "Game over! Come back again...\n";
                exit(0);
            }

            clear_screen();
            cout << "Your move at " << person_rc << " was valid.\n";

            break;
        case marker::empty:
            cout << "Error: Player marker for human player cannot be empty.\n";
            exit(-1);
        }

        // test for a winner
        if (move_count >= (edge_len + edge_len - 1)) {
            winning_side = who_won(); // result is marker::empty, marker::playerX, or marker::playerO

            if (enum2int(winning_side)) { // e.g., wasn't marker::empty
                cout << "We have a winner. "
                     << (winning_side == person_marker ? "You won. Congratulations!" : " The computer beat you )-:")
                     << "\nGame over. Come back and play again!\n\n";
                display_board();
                break;
            }
        }
    }
}


bool inline Hex::is_in_start(int idx, marker side) const
{
    if (side == marker::playerX) {
        return idx < edge_len;
    }
    else if (side == marker::playerO) {
        return idx % edge_len == 0;
    }
    else
        cout << "Error: invalid side. Must be " << marker::playerX << " or " << marker::playerO << ".\n";
    exit(-1);
}


bool inline Hex::is_in_finish(int idx, marker side) const
{
    if (side == marker::playerX) {
        return idx > max_idx - edge_len - 1;
    }
    else if (side == marker::playerO) {
        return idx % edge_len == edge_len - 1;
    }
    else 
        cout << "Error: invalid side. Must be " << marker::playerX << " or " << marker::playerO << ".\n";
    exit(-1);
}

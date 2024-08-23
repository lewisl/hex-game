##########################################################################
#             Class Hex game playing procs
##########################################################################

import 
  random,
  strutils,
  times,
  std/deques,
  std/rdstdin, # read stdin ignoring control keys
  hex_board,
  graph,
  helpers

# simulate a hex game by filling empty positions with shuffled markers (doesn't include the test move)
proc simulate_hexboard_positions[T](hb: var Hexboard, empties: var seq[T])  =  
  
  # var foo = empties  # create a non-destructive shuffle=>must discard in the caller
  shuffle(empties)   # does not include current test move

  var
    current: Marker = playerO  # computer first after person's move
    next: Marker    = playerX
  
  for pos in empties:
    hb.set_hex_marker(pos, current)
    swap(current, next) # alternate the markers to be placed on the board at each position


proc fill_board(hb: var Hexboard, indices: seq[int], value: Marker)  =
  for idx in indices:
    hb.set_hex_marker(idx, value)


proc is_in_start(hb: Hexboard, idx: int, side: Marker) : bool =
  if side == Marker.playerX:
    return idx < hb.edge_len
  elif side == Marker.playerO:
    return idx mod hb.edge_len == 0
  else:
    raise newException(ValueError, "Error: Invalid side: must be Marker.playerX or Marker.playerO\n")


# depth first search from positions in finish_border to connected graph to a move in the start_border
proc find_ends(hb: var Hexboard, side: Marker, whole_board: bool = false) : Marker =
  hb.winner_assess_time_t0 = cpuTime()

  # clear instead of create new containers
  hb.captured.setLen(0)
  hb.possibles.clear()

  for pos in hb.finish_border[ord(side)]:
    if hb.get_hex_marker(pos) == side:
      hb.possibles.addlast(pos)
      hb.captured.add(pos)

  while not (hb.possibles.len == 0):
    if hb.is_in_start(hb.possibles[0], side):
      # hb.winner_assess_time_cum += cpuTime() - hb.winner_assess_time_t0
      return side

    # find neighbors of the current node that match the current side and exclude already captured nodes
    hb.neighbors = get_neighbor_nodes(hb.hex_graph, hb.possibles[0], side, hb.captured)  

    if hb.neighbors.len == 0:
      if not hb.possibles.len == 0:
        hb.possibles.popFirst()
      break
    else:
      hb.possibles[0] = hb.neighbors[0]
      hb.captured.add(hb.neighbors[0])

      for i in 1 ..< hb.neighbors.len:
        hb.possibles.addlast(hb.neighbors[i])
        hb.captured.add(hb.neighbors[i])

  hb.winner_assess_time_cum += (cpuTime() - hb.winner_assess_time_t0)
  if whole_board:
    return (if side == playerO: playerX else: playerO)
  else:
    return Marker.empty

# makes new copy of destination--probably returns via move semantics
proc copy_except[T](source: seq[T], exclude: int) : seq[T] =
  assert exclude in 0 ..< source.len
  result = newSeq[T](source.len - 1)   # magic variable is returned
  var j = 0  # j is index counter for result
  for i in 0 ..< source.len:  # i is index counter for source
    if i == exclude:
      discard
    else:
      result[j] = source[i]
      inc j


proc monte_carlo_move(hb: var Hexboard, side: Marker, n_trials: int) : RowCol =

  hb.move_sim_time_t0 = cpuTime()

  # method uses class fields: clear them instead of creating new objects each time
  hb.win_pct_per_move.setLen(0)

  var
    wins: int
    winning_side: Marker = empty
    best_move: int

  var current_move = -1  # keep track of index in empty_idxs=>NOT same as loop counter

  # loop over the available move positions: make tst move, setup positions to randomize
  for tst_move in hb.empty_idxs:  # tst_move is an empty position for simulating test computer moves 
    inc current_move  # index to the container empty_idxs NOT = to the tst_move position
    hb.set_hex_marker(tst_move, side) # set the test move on the board

    wins = 0
    hb.shuffle_idxs = hb.empty_idxs.copy_except(current_move)

    for trial in 0 ..< n_trials:
      hb.simulate_hexboard_positions(hb.shuffle_idxs)  # argument is mutable!
      winning_side = hb.find_ends(side, true)
      wins += (if winning_side == side: 1 else: 0)

    hb.win_pct_per_move.add(wins.toBiggestFloat / n_trials.toBiggestFloat) # calculate and save computer win pct.

    hb.set_hex_marker(tst_move, Marker.empty)  # reverse the trial move

  # after all test moves have been tried, find the maximum computer win pct across them
  var  maxpct: float64 = 0.0
  var choice: int = 0
  best_move = hb.empty_idxs[0]
  for i in 0 ..< hb.win_pct_per_move.len:
    # echo "win % ", hb.win_pct_per_move, " i ", i, " move: ", hb.empty_idxs[i], "\n"
    if hb.win_pct_per_move[i] > maxpct:
      maxpct = hb.win_pct_per_move[i]
      best_move = hb.empty_idxs[i]  
      choice  = i

  # echo "************ ", "choice ", choice, " maxpct ", maxpct, " move ", hb.empty_idxs[choice]

  hb.fill_board(hb.empty_idxs, Marker.empty) # restore board to move state before simulation
  hb.move_sim_time_cum += cpuTime() - hb.move_sim_time_t0
  return hb.l2rc(best_move)


# one proc to do a move: set the marker, remove the empty index, increment the move counter
proc do_move(hb: var Hexboard, rc: Rowcol, side: Marker) =
  hb.set_hex_marker(rc, side)
  hb.empty_idxs.delete(hb.empty_idxs.find(hb.rc2l(rc)))  # update list of empty board positions
  inc hb.move_count


proc computer_move(hb: var Hexboard, side: Marker, n_trials: int) : RowCol =
  var 
    rc: RowCol
    
  rc = hb.monte_carlo_move(side, n_trials)
  hb.do_move(rc, side)
  return rc


# prompting sequence for human player's move
proc move_input(msg: string) : RowCol =
  var 
    row, col, cnt: int
    input: string
    more_input = true

  while more_input:
    input = readLineFromStdin("row col: ")
    cnt = 1
    for item in splitWhitespace(input):  # lot'o'rigamorole to avoid splitting input into a seq
      if cnt == 1:
        row = try: item.parseInt
            except ValueError as e:
              echo "  *** ", e.msg & ". Move inputs must be integers..."
              break
        cnt = 2
      elif cnt == 2:
        col = try: item.parseInt
            except ValueError as e:
              echo "  *** ", e.msg & ". Move inputs must be integers..."
              break
        more_input = false
        break
  
  return RowCol(row: row, col: col)


proc is_valid_move(hb: var Hexboard, rc: RowCol) : bool =
  var
    row: int = rc.row
    col: int = rc.col
    valid_move = true
  let
    bad_position: string = "Your move used an invalid row or column.\n\n"
    not_empty: string = "Your move didn't choose an empty position.\n\n"
  var
    msg: string = ""

  if row > hb.edge_len or row < 1:
    valid_move = false
    msg = bad_position
  elif col > hb.edge_len or col < 1:
    valid_move = false
    msg = bad_position
  elif hb.get_hex_marker(rc) != Marker.empty:
    valid_move = false
    msg = not_empty

  echo(msg)
  return valid_move


proc person_move(hb: var Hexboard, side: Marker) : RowCol =
  # need to implement writing to a file
  var
    rc: RowCol
    valid_move: bool = false

  while not valid_move:
    echo("Enter a move in an empty position that contains '.'\n")
    echo("Enter your move as the row number and the column number, separated by a space.\n")
    echo("The computer prompts row col:  and you enter 3 5, followed by the enter key.\n")
    echo("Enter -1 -1 to quit...\n")

    rc = move_input("Please enter 2 integers: ")

    if rc.row == -1 or rc.col == -1:
      rc.row = -1 
      rc.col = -1
      return rc

    if rc.row == -5:
      let 
        date = now()
        filename = "board graph " & $date & ".txt"
        f = open(filename, fmWrite) # filename will almost always be unique
      display_graph(hb.hex_graph, f)
      f.close

    valid_move = hb.is_valid_move(rc)

  hb.do_move(rc, side)
  return rc


proc who_won(hb: var Hexboard) : Marker =
  var winner: Marker = empty
  let sides: array = [Marker.playerX, Marker.playerO]

  for side in sides:
    winner =  hb.find_ends(side)
    if winner != Marker.empty:
      break
  return winner


proc who_goes_first() : tuple[person_marker: Marker, computer_marker: Marker] =
  # playerX is always first; playerO is always second. Who gets each marker?
  while true:
    write(stdout, repeat("\n", 15))
    # write(stdout, "*** Do you want to go first? (enter y or yes or n or no) ")
    let answer = readLineFromStdin("*** Do you want to go first? (enter y or yes or n or no) ")

    if contains("yes", answer.toLowerAscii):
      # person_marker = Marker.playerX
      # computer_marker = Marker.playerO

      echo("\nYou go first playing X Markers.")
      echo("Make a path from the top row to the bottom.")
      echo("The computer goes second playing O markers.")
      echo("\n") # 2 blank lines

      return (person_marker: Marker.playerX, computer_marker: Marker.playerO)
    elif contains("no", answer.toLowerAscii):
      # person_marker = Marker.playerO
      # computer_marker = Marker.playerX

      echo("\nThe computer goes first playing X Markers.")
      echo("You go second playing O Markers.")
      echo("Make a path from the first column to the last column.")
      echo("\n") # 2 blank lines

      return (person_marker: Marker.playerO, computer_marker: Marker.playerX)
    else:
      echo("    Please enter [y]es or [n]o")


proc play_game*(hb: var Hexboard, n_trials: int) =
  var
    person_rc: RowCol  # person's move position
    computer_rc: RowCol  # computer's move position
    person_marker: Marker   # marker used by human player
    computer_marker: Marker  # marker used by computer player
    winning_side: Marker

  randomize()       # set seed for random module procs
  clear_screen()
  echo("\n")

  let tp = who_goes_first() # possible with anon tuple, but this is order independent
  person_marker = tp.person_marker; computer_marker = tp.computer_marker

  hb.move_count = 0

  while true:   # move loop: Marker.playerX always first, whether person or computer
    case person_marker   # human goes first playing marker playerX
      of Marker.playerX:
        hb.display_board()
        person_rc = hb.person_move(person_marker)
        if person_rc.row == -1:
          echo("Game over! Come back again...")
          quit()

        computer_rc = hb.computer_move(computer_marker, n_trials)
        clear_screen()
        echo("Your move at ", $person_rc, " was valid.")
        echo("The computer moved at ", $computer_rc, ".\n\n")

      of Marker.playerO:    # human goes second playing marker playerO
        computer_rc = hb.computer_move(computer_marker, n_trials)
        echo("The computer moved at ", $computer_rc, ".\n")
        hb.display_board()

        person_rc = hb.person_move(person_marker)
        if (person_rc.row == -1):
          echo("Game over! Come back again...")
          quit()

        clear_screen()
        echo("Your move at ", $person_rc, " was valid.")
        break

      of Marker.empty:
        raise newException(ValueError, "Error: Player Marker for human player cannot be empty.\n")

    # test for winner
    if hb.move_count >= (hb.edge_len + hb.edge_len - 1):  # minimum no. of moves to complete a path from start to end borders
      winning_side = hb.who_won()

      if winning_side == Marker.playerO or winning_side == Marker.playerX:  # equivalent to ord(winning_side) > 0  
        echo("We have a winner. ")
        if winning_side == person_marker:
          write(stdout, "You won. Congratulations!\n")
        else:
          write(stdout, "The computer beat you )-:")
          echo("\nGame over. Come back and play again!\n")
        hb.display_board()
        break

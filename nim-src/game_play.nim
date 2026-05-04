##########################################################################
#             Class Hex game playing procs
##########################################################################

import 
  random,
  strutils,
  times,
  std/deques,
  std/enumerate,
  std/rdstdin, # read stdin ignoring control keys
  
  # project modules
  hex_board,
  graph,
  helpers


type 
  PlayerKind  = enum
    plPerson
    plComputer

type
  Mover = ref object   # object is always passed by mutable reference
    role:        PlayerKind
    move_rc:     RowCol
    marker:      Marker
    victory_msg: string

var person = Mover(
  role:        PlayerKind.plPerson,
  move_rc:     RowCol(row: 1, col: 1),
  marker:      Marker.playerX,
  victory_msg: "You won. Congratulations!\n\n"
)

var computer = Mover(
  role:         PlayerKind.plComputer,
  move_rc:      RowCol(row: 1, col: 1),
  marker:       Marker.playerO,
  victory_msg:  "The computer beat you )-:\n\n"
)

# simulate a hex game by filling empty positions with shuffled markers (doesn't include the test move)
proc simulate_hexboard_positions(hb: var Hexboard, computer_side: Marker) =   
  var 
    current: Marker = if computer_side == playerX: playerO else: playerX 
    next: Marker = computer_side  # second in simulation because computer already made a test move
  shuffle(hb.throw_away)
  for idx in hb.throw_away:
    hb.set_hex_marker(idx, current)
    swap(current, next)


proc fill_board(hb: var Hexboard, indices: seq[int], value: Marker)  =
  for idx in indices:
    hb.set_hex_marker(idx, value)


proc is_in_start(hb: Hexboard, idx: int, side: Marker) : bool =
  if side == Marker.playerX:
    return idx < hb.edge_len  # always plays first, so start is first row
  elif side == Marker.playerO:
    return idx mod hb.edge_len == 0  # always plays second, so start is first column
  else:
    raise newException(ValueError, "Error: Invalid side: must be Marker.playerX or Marker.playerO\n")


# depth first search from positions in finish_border to connected graph to a move in the start_border
proc find_ends(hb: var Hexboard, side: Marker, whole_board: bool = false) : Marker =

  # clear instead of create new containers
  hb.captured.setLen(0)
  hb.possibles.clear()

  for pos in hb.finish_border[ord(side)]:
    if hb.get_hex_marker(pos) == side:
      hb.possibles.addlast(pos)
      hb.captured.add(pos)

  while hb.possibles.len > 0:

    while true:
      if hb.is_in_start(hb.possibles[0], side):
        return side

      # find neighbors of the current node that match the current side and exclude already captured nodes
      hb.neighbors = get_neighbor_nodes(hb.hex_graph, hb.possibles[0], side, hb.captured)  

      if hb.neighbors.len == 0:
        if hb.possibles.len > 0:
          hb.possibles.popFirst()
        break    
      else:
        hb.possibles[0] = hb.neighbors[0]
        hb.captured.add(hb.neighbors[0])

        for i in 1 ..< hb.neighbors.len:
          hb.possibles.addlast(hb.neighbors[i])
          hb.captured.add(hb.neighbors[i])

  if whole_board:
    return (if side == playerO: playerX else: playerO)
  else:
    return Marker.empty


proc monte_carlo_move(hb: var Hexboard, side: Marker, n_trials: int) : RowCol =

  hb.move_sim_time_t0 = cpuTime()
  var
    wins: int
    winning_side: Marker = empty

  hb.wins_per_move.setLen(0) # clear class member, don't create new object
  hb.shuffle_idxs.setLen(0)

  # pick a test move, run the trials, track wins, reset test move, pick best move
  for move_num, tst_move in hb.empty_idxs:  # move_num indexes the seq; tst_move are values in the seq
    hb.set_hex_marker(tst_move, side) 
    wins = 0

    # hb.shuffle_idxs has to be stable for this to work--can't be shuffled
    if move_num == 0:
      for i in 0 ..< hb.empty_idxs.len - 1:
        hb.shuffle_idxs.add((hb.empty_idxs[i+1]))  # add all but empty_idxs[0] 1st time
    else:       # very fast to swap 2 positions                                 
      hb.shuffle_idxs[move_num - 1] = hb.empty_idxs[move_num - 1]  # copy 1 index, excludes empty at max index

    hb.throw_away = hb.shuffle_idxs  # make copy once per move, not once per trial

    for trial in 0 ..< n_trials:
      hb.simulate_hexboard_positions(side)  # object hb contains everything needed
      winning_side = hb.find_ends(side, true)
      wins += (if winning_side == side: 1 else: 0)

    hb.wins_per_move.add(wins)   
    hb.set_hex_marker(tst_move, Marker.empty)  # reverse the trial move

  # find the maximum wins across all test moves to select the best test move
  var  
    maxwins = hb.wins_per_move[0]
    choice = 0
    best_move = hb.empty_idxs[0]
  for i in 0 ..< hb.wins_per_move.len:
    # echo "win % ", hb.wins_per_move, " i ", i, " move: ", hb.empty_idxs[i], "\n"
    if hb.wins_per_move[i] > maxwins:
      maxwins = hb.wins_per_move[i]
      best_move = hb.empty_idxs[i]  
      choice  = i
  # echo "************ ", "choice ", choice, " maxpct ", maxpct, " move ", hb.empty_idxs[choice]
  hb.fill_board(hb.empty_idxs, Marker.empty) # restore board to state before simulation
  hb.move_sim_time_cum += cpuTime() - hb.move_sim_time_t0
  return hb.l2rc(best_move)


# one proc to do a move: set the marker, remove the empty index, increment the move counter
proc do_move(hb: var Hexboard, rc: Rowcol, side: Marker) =
  let linear = hb.rc2l(rc)
  # assert linear in hb.empty_idxs
  let empty_idx = hb.empty_idxs.find(linear) # index in empty_idxs of the new move position
  # assert empty_idx != -1, "new move was not to an empty\n"
  # assert hb.is_empty(linear), "Move is not to empty position" # the new move position has to be empty on the board

  hb.set_hex_marker(linear, side)
  hb.empty_idxs.delete(empty_idx)  # remove move from empties
  hb.move_history.add(Move(player: side, row: rc.row, col: rc.col))
  inc hb.move_count


proc is_valid_move(hb: Hexboard, rc: RowCol) : bool =
  var
    row: int = rc.row
    col: int = rc.col
    valid_move = true
    msg: string = ""
  let
    bad_position: string = "Your move used an invalid row or column.\n\n"
    not_empty: string = "Your move didn't choose an empty position.\n\n"
    
  if row > hb.edge_len or row < 1:
    valid_move = false
    msg = bad_position
  elif col > hb.edge_len or col < 1:
    valid_move = false
    msg = bad_position
  elif hb.get_hex_marker(rc) != Marker.empty:
    valid_move = false
    msg = not_empty

  if msg != "":
    echo(msg)
  return valid_move


proc computer_move(hb: var Hexboard, side: Marker, n_trials: int) : RowCol =
  let rc = hb.monte_carlo_move(side, n_trials)
  assert hb.is_valid_move(rc)
  hb.do_move(rc, side)
  return rc


# prompting sequence for human player's move
proc move_input(msg: string) : RowCol =
  var 
    rcinput = [0,0]
    input: string
    more_input = true
  while more_input:
    input = readLineFromStdin("row col: ")
    for cnt, item in enumerate(splitWhitespace(input)):  # lot'o'rigamorole to avoid splitting input into a seq
      rcinput[cnt] = try: item.parseInt
          except ValueError as e:
            echo "  *** ", e.msg & ". Move inputs must be integers..."
            break
      if cnt == 1:  # ignore more inputs than 2 or inputs done
        more_input = false
        break
  return RowCol(row: rcinput[0], col: rcinput[1])


proc save_diagnostics*(hb: Hexboard)  # forward declaration


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
      hb.save_diagnostics()
    valid_move = hb.is_valid_move(rc)
  hb.do_move(rc, side)
  return rc


proc who_won(hb: var Hexboard, this_side: Marker) : Marker =
  var winner: Marker = empty
  winner =  hb.find_ends(this_side)  # finds either player marker or Marker.empty
  return winner


proc who_goes_first() : array[2, Mover] =
  # playerX is always first; playerO is always second. Who gets each marker?
  while true:
    write(stdout, repeat("\n", 15))
    # write(stdout, "*** Do you want to go first? (enter y or yes or n or no) ")
    let answer = 
      readLineFromStdin("*** Do you want to go first? (enter y or yes or n or no) ").strip.toLowerAscii

    case answer
    of "yes", "y":
      echo("\nYou go first playing X Markers.")
      echo("Make a path from the top to bottom or bottom to top. Connect along the lines.")
      echo("The computer goes second playing O markers.")
      echo("The computer tries to make a path from side to side.")
      echo("\n") # 2 blank lines
      person.marker = Marker.playerX
      computer.marker = Marker.playerO
      return [person, computer]

    of "no", "n":
      echo("\nThe computer goes first playing X Markers.")
      echo("The computer tries to make a path from side to side.")
      echo("You go second playing O Markers.")
      echo("Make a path sideways from the first column to the last column.")
      echo("\n") # 2 blank lines
      person.marker = Marker.playerO
      computer.marker = Marker.playerX
      return [computer, person]

    else:
      echo("    Please enter [y]es or [n]o")


proc play_game*(hb: var Hexboard, n_trials: int, debug: bool = false) =
  var
    winning_side: Marker
  randomize()       # set seed for random module procs
  if not debug:  clear_screen()
  echo("\n")

  let movers = who_goes_first() # [computer, person] or [person, computer]

  hb.move_count = 0

  block gameLoop:
    while true:   
      for mover in movers:
        case mover.role
          of plPerson:
            hb.display_board()   
            mover.move_rc = hb.person_move(mover.marker)
          of plComputer:
            mover.move_rc = hb.computer_move(mover.marker, n_trials)
        if mover.move_rc.row == -1:
          echo("Game Over! Come back again...")
          quit()

        if hb.move_count >= (hb.edge_len + hb.edge_len - 1):
          hb.winner_assess_time_t0 = cpuTime()
          winning_side = hb.who_won(mover.marker)  # evaluate only the current player
          hb.winner_assess_time_cum += cpuTime() - hb.winner_assess_time_t0

          if winning_side == mover.marker:
            clear_screen()
            echo("We have a winner. ")
            write(stdout, mover.victory_msg)
            hb.display_board()
            echo("Game over. Come back and play again!\n")
            break gameLoop  # without explicit block, nim breaks only innermost enclosing loop

      if not debug: clear_screen()
      echo("Your move at ", $person.move_rc, " was valid.")
      echo("The computer moved at ", $computer.move_rc, ".\n\n")

      if not debug: clear_screen()
      echo("Your move at ", $person.move_rc, " was valid.")
      echo("The computer moved at ", $computer.move_rc, ".\n\n")

proc save_diagnostics*(hb: Hexboard) =
  let 
    date = now()
    filename = "diagnostics " & $date & ".txt"
    f = open(filename, fmWrite) # filename will almost always be unique
  write(f, "***** Graph map\n")
  display_graph(hb.hex_graph, f)
  write(f, "\n***** Move History\n")
  display_move_history(hb, hb.move_history, f)
  write(f, "\n***** Empty positions\n")
  write(f, $hb.empty_idxs)
  f.close
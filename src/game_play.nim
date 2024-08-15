##########################################################################
#             Class Hex game playing procs
##########################################################################

import hex_board
import graph
import random
import strutils
import times
import std/deques
import helpers
import std/rdstdin # read stdin and don't read control keys



proc simulate_hexboard_positions(hb: var Hexboard) =  # , empty_hex_positions: var seq[int] 
  # var randgen = initRand(1_010_010)
  shuffle(hb.empty_pos) 

  var
    current: Marker = playerO
    next: Marker    = playerX
  
  for i in 0 ..< hb.empty_pos.len:
    hb.set_hex_marker(hb.hex_graph, hb.empty_pos[i], current)
    swap(current, next) # swap the markers to be placed on the board each iteration


proc fill_board(hb: var Hexboard, indices: seq[int], value: Marker)  =
  for idx in indices:
    # hb.hex_graph.node_data[idx] = value
    hb.hex_graph.set_node_data(idx, value)

proc random_move(hb: var Hexboard) : RowCol =
  var
    rc: RowCol
    maybe: int

  # var randgen = initRand(rand(1_010_010))
  shuffle(hb.rand_nodes)

  for i in 0 ..< hb.max_idx:
    maybe = hb.rand_nodes[i]
    if hb.is_empty(maybe):
      rc = hb.linear2rowcol(maybe)
      break

  if rc.row == 0 and rc.col == 0: # never found an empty position
    rc.row = -1
    rc.col = -1 # sentinel for invalid move that will end game

  return rc
  

proc naive_move(hb: var Hexboard, side: Marker) : RowCol =
  var
    rc, prev_move: RowCol
    prev_move_linear: int
    neighbor_nodes: seq[int]

  # var randgen = initRand(rand(1_010_010))

  if hb.move_seq[ord(side)].len == 0:
    shuffle(hb.start_border[ord(side)])
    for maybe in hb.start_border[ord(side)]:
      if hb.is_empty(maybe):
        rc = hb.linear2rowcol(maybe)
        return rc

  else:
    prev_move = hb.move_seq[ord(side)][^1]
    prev_move_linear = hb.rowcol2linear(prev_move)

    neighbor_nodes = hb.hex_graph.get_neighbor_nodes(prev_move_linear, Marker.empty)
    if neighbor_nodes.len == 0:
      return hb.random_move()

    shuffle(neighbor_nodes)
    for node in neighbor_nodes:
      rc = hb.linear2rowcol(node)
      if rc.col > prev_move.col:
        return rc
    rc = hb.linear2rowcol(neighbor_nodes[neighbor_nodes.high])

  return rc


proc is_in_start(hb: Hexboard, idx: int, side: Marker) : bool =
  if side == Marker.playerX:
    return idx < hb.edge_len
  elif side == Marker.playerO:
    return idx mod hb.edge_len == 0
  else:
    raise newException(ValueError, "Error: Invalid side: must be Marker.playerX or Marker.playerO\n")


proc find_ends(hb: var Hexboard, side: Marker, whole_board: bool = false) : Marker =
  hb.winner_assess_time_t0 = cpuTime()
  var 
    front = 0
    possibles = initDeque[int](hb.max_idx-1)

  # clear instead of create new containers
  hb.neighbors.setLen(0)
  hb.captured.setLen(0)

  for pos in hb.finish_border[ord(side)]:
    if hb.hex_graph.get_hex_marker(pos) == side:
      possibles.addlast(pos)
      hb.captured.add(pos)

  while not (possibles.len == 0):
    if hb.is_in_start(possibles[0], side):
      # hb.winner_assess_time_cum += cpuTime() - hb.winner_assess_time_t0
      return side

    # find neighbors of the current node that match the current side and exclude already captured nodes
    hb.neighbors = hb.hex_graph.get_neighbor_nodes(possibles[front], side, hb.captured)

    if hb.neighbors.len == 0:
      if not possibles.len == 0:
        possibles.popfirst()
      break
    else:
      possibles[front] = hb.neighbors[0]
      hb.captured.add(hb.neighbors[0])

      for i in 1 ..< hb.neighbors.len:
        possibles.addlast(hb.neighbors[i])
        hb.captured.add(hb.neighbors[i])

  hb.winner_assess_time_cum += (cpuTime() - hb.winner_assess_time_t0)
  if whole_board:
    return (if side == playerO: playerX else: playerO)
  else:
    return Marker.empty


proc monte_carlo_move(hb: var Hexboard, side: Marker, n_trials: int) : RowCol =

  hb.move_sim_time_t0 = cpuTime()

  # no more: method uses class fields: clear them instead of creating new objects each time
  hb.empty_pos = newSeq[int](0)
  hb.random_pos = newSeq[int](0)
  hb.win_pct_per_move.setLen(0)

  var
    wins: int
    winning_side: Marker = empty
    best_move: int

  # loop over board positions to find empty positions
  for i in 0 ..< hb.max_idx:
    if hb.is_empty(i):
      hb.empty_pos.add(i)

  var random_pos = newSeq[int](hb.empty_pos.len - 1)

  # loop over the available move positions: make tst move, setup positions to randomize

  for tst_move_no in 0 ..< hb.empty_pos.len:

    hb.set_hex_marker(hb.hex_graph, hb.empty_pos[tst_move_no], side) # set computer's move to evaluate
    wins = 0

    for idx in 0 ..< hb.empty_pos.len:
      if idx > random_pos.len:
        break
      elif idx < tst_move_no:
        random_pos[idx] = hb.empty_pos[idx]

      elif idx == tst_move_no:
        continue
      else:
        random_pos[idx-1] = hb.empty_pos[idx]

    for trial in 0 ..< n_trials:
      hb.simulate_hexboard_positions()

      winning_side = hb.find_ends(side, true)

      wins += (if winning_side == side: 1 else: 0)

    hb.win_pct_per_move.add(wins.toFloat / n_trials.toFloat) # calculate and save computer win pct.

    # reverse the trial move
    hb.set_hex_marker(hb.hex_graph, hb.empty_pos[tst_move_no], Marker.empty)

  # find the maximum computer win pct across all simulated moves
  var  maxpct: float= 0.0
  best_move = hb.empty_pos[0]
  for i in 0 ..< hb.win_pct_per_move.len:
    if hb.win_pct_per_move[i] > maxpct:
      maxpct = hb.win_pct_per_move[i]
      best_move = hb.empty_pos[i]

  hb.fill_board(hb.empty_pos, Marker.empty) # restore board to real move state

  hb.move_sim_time_cum += cpuTime() - hb.move_sim_time_t0

  return hb.linear2row_col(best_move)


proc computer_move(hb: var Hexboard, side: Marker, how: Do_move, n_trials: int) : RowCol =
  var rc: RowCol

  case(how)
  of naive:
    rc = hb.naive_move(side)
  of monte_carlo:
    rc = hb.monte_carlo_move(side, n_trials)

  hb.set_hex_marker(hb.hex_graph, rc, side)
  hb.move_seq[ord(side)].add(rc)

  hb.move_count.inc
  return rc


# prompting sequence for human player's move
proc move_input(msg: string) : RowCol =
  var 
    row, col: int
    input: string
    items: seq[string]

  while true:
    input = readLineFromStdin("row col: ")
    items = input.splitWhitespace()
    if items.len != 2:
      write(stdout, msg)
      continue
    else:
      row = items[0].parseInt
      col = items[1].parseInt
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
  elif hb.get_hex_marker(hb.hex_graph, rc) != Marker.empty:
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
    # write(stdout, "row col: ")

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

  hb.set_hex_marker(hb.hex_graph, rc, side)
  hb.move_seq[ord(side)].add(rc)

  inc hb.move_count
  return rc


proc who_won(hb: var Hexboard) : Marker =
  var winner: Marker = empty
  let sides: array = [Marker.playerX, Marker.playerO]

  for side in sides:
    winner =  hb.find_ends(side)
    if winner != Marker.empty:
      break
  return winner


proc play_game*(hb: var Hexboard, how: Do_move, n_trials: int) =
  var
    person_rc: RowCol # person's move position
    computer_rc: RowCol # computer's move position
    answer: string
    person_marker: Marker
    computer_marker: Marker
    winning_side: Marker

  randomize()
  clear_screen()
  echo("\n")

  # who goes first?
  while true:
    write(stdout, repeat("\n", 15))
    # write(stdout, "*** Do you want to go first? (enter y or yes or n or no) ")
    answer = readLineFromStdin("*** Do you want to go first? (enter y or yes or n or no) ")

    if contains("yes", answer.toLowerAscii):
      person_marker = Marker.playerX
      computer_marker = Marker.playerO

      echo("\nYou go first playing X Markers.")
      echo("Make a path from the top row to the bottom.")
      echo("The computer goes second playing O markers.")
      echo("\n") # 2 blank lines

      break
    elif contains("no", answer.toLowerAscii):
      person_marker = Marker.playerO
      computer_marker = Marker.playerX

      echo("\nThe computer goes first playing X Markers.")
      echo("You go second playing O Markers.")
      echo("Make a path from the first column to the last column.")
      echo("\n") # 2 blank lines

      break
    else:
      echo("    Please enter [y]es or [n]o")

  hb.move_count = 0

  while true:   # move loop
    case person_marker
    of Marker.playerX:
      hb.display_board()
      person_rc = hb.person_move(person_marker)
      if person_rc.row == -1:
        echo("Game over! Come back again...")
        quit()

      computer_rc = hb.computer_move(computer_marker, how, n_trials)
      clear_screen()
      echo("The computer moved at ", $computer_rc, ".")
      echo("Your move at ", $person_rc, " was valid.\n\n")

    of Marker.playerO:
      computer_rc = hb.computer_move(computer_marker, how, n_trials)
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
    if hb.move_count >= (hb.edge_len + hb.edge_len - 1):
      winning_side = hb.who_won()

      if ord(winning_side) > 0:
        echo("We have a winner. ")
        if winning_side == person_marker:
          write(stdout, "You won. Congratulations!\n")
        else:
          write(stdout, "The computer beat you )-:")
          echo("\nGame over. Come back and play again!\n")
        hb.display_board()
        break



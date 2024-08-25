
import 
  strutils,
  sequtils,
  std/deques,
  std/strformat,
  # project module
  graph

type 
  Marker*  = enum
    empty
    playerX
    playerO

type  
  RowCol* = object
    row*: int = 0
    col*: int = 0

type Move* = object
  player*: Marker
  row*: int
  col*: int

type
  Hexboard* = object
    # used by hex_board
    hex_graph*:           Graph[Marker]
    edge_len*:            int
    max_idx*:             int
    rand_nodes*:          seq[int]
    start_border*:        array[1..2, seq[int]]  # start border for each player
    finish_border*:       array[1..2, seq[int]]  # finish border for each player
    positions*:           ref seq[Marker]   # traced reference to node_data in Graph
    # used by game_play: 
      # using this object reduces game_play allocations of many small seqs  
    move_count*:          int
    move_history*:        seq[Move]
    wins_per_move*:       seq[int]
    neighbors*:           seq[int]
    captured*:            seq[int]
    possibles*:           Deque[int]  # candidates for finding connected path from edge to edge
    shuffle_idxs*:        seq[int]
    empty_idxs*:          seq[int]
    winner_assess_time_t0*:  float    # enables time measurements to be spread across procs
    winner_assess_time_cum*: float
    move_sim_time_t0*:       float
    move_sim_time_cum*:      float

proc make_hex_graph*(hb: var Hexboard)  # forward declaration

proc initHexboard*(edge_len: int) : Hexboard =  # in c++ terms, a custom constructor
  var 
    edge_len = edge_len
    max_idx = edge_len * edge_len
  var hb = Hexboard(edge_len: edge_len, 
              max_idx: max_idx,
              empty_idxs: (0..(max_idx-1)).toSeq,  # initialize to all positions empty, length is max_idx, memory allocated
              shuffle_idxs: newSeqOfCap[int](max_idx),  # reserve the memory, but length is zero
              possibles: initDeque[int](max_idx-1),
              hex_graph: initGraph[Marker](max_idx, Marker.empty))
  hb.positions = hb.hex_graph.node_data  # alias for hex_graph.node_data: base addr of positions traces base addr of hex_graph.node_data
  for i in 0..max_idx-1:
    hb.rand_nodes.add(i)
  return hb

# short for rowcol2linear: conversions between row/col indices and ordinal integer indices to board positions
proc rc2l*(hb: Hexboard, row: int, col: int) : int  =
  let r = row - 1
  let c = col - 1

  if (r < hb.edge_len) and (c < hb.edge_len):
    return (r * hb.edge_len) + c
  else:
    raise newException(ValueError, "Bad row or col input: both must be >= edge length")

proc rc2l*(hb: Hexboard, rc: RowCol) : int  =
  return hb.rc2l(rc.row, rc.col)

proc l2rc*(hb: Hexboard, linear: int) : RowCol  =
  if linear < hb.max_idx:
    return RowCol(row: (linear div hb.edge_len) + 1, col: (linear mod hb.edge_len) + 1)
  else:
    raise newException(ValueError, "Error: linear index input greater than edge_len * edge_len." )

proc is_empty*(hb: Hexboard, linear: int) : bool =
  return hb.positions[linear] == Marker.empty

proc is_empty*(hb: Hexboard, rc: RowCol) : bool =
  return hb.positions[hb.rc2l(rc)] == Marker.empty


# getters and setters for hex_board from ref to Graph.node_data
proc set_hex_marker*(hb: var Hex_board, rc: RowCol, val: Marker)  = 
  hb.positions[hb.rc2l(rc)] = val
proc set_hex_marker*(hb: var Hex_board,  row: int, col: int, val: Marker)  =
  hb.positions[hb.rc2l(row, col)] = val
proc set_hex_marker*(hb: var Hex_board,  linear: int, val: Marker)  =
  hb.positions[linear] = val

proc get_hex_Marker*(hb: Hex_board,  rc: RowCol) : Marker  =
  return hb.positions[hb.rc2l(rc)]
proc get_hex_Marker*(hb: Hex_board,  row: int, col: int) : Marker  =
  return hb.positions[hb.rc2l(row, col)]
proc get_hex_Marker*(hb: Hex_board,  linear: int) : Marker  =
  return hb.positions[linear]


proc lead_space(row: int): string =
  return repeat(" ", row * 2)


# create lists of the indices of positions in the borders of the board
proc define_borders(hb: var Hexboard) =
    
  # top border
  for col in 1..hb.edge_len:
    var row: int = 1
    hb.start_border[ord(Marker.playerX)].add(hb.rc2l(row, col))
  
  # bottom border
  for col in 1..hb.edge_len:
    let row = hb.edge_len
    hb.finish_border[ord(Marker.playerX)].add(hb.rc2l(row, col))

  # left border
  for row in 1..hb.edge_len:
    let col = 1
    hb.start_border[ord(Marker.playerO)].add(hb.rc2l(row, col))
  
  # right border
  for row in 1..hb.edge_len:
    let col = hb.edge_len
    hb.finish_border[ord(Marker.playerO)].add(hb.rc2l(row, col))


## create graph of hexboard positions
## add graph edges for adjacent hexes based on the layout of a Hex game
##   linear indices run from 0 at upper, left then across the row,
##   then down 1 row at the left edge, and across, etc.
proc make_hex_graph*(hb: var Hexboard) =

  hb.define_borders() # set positions that are in the top, right, bottom, and left borders

  #4 corners of the board: 2 or 3 edges per node                            
  #upper left
  add_edge(hb.hex_graph, node=hb.rc2l(1, 1), tonodes =[hb.rc2l(2,1), hb.rc2l(1,2)])
  # upper right
  add_edge(hb.hex_graph, node=hb.rc2l(1, hb.edge_len), 
          tonodes=[hb.rc2l(1, (hb.edge_len - 1)), hb.rc2l(2, hb.edge_len), hb.rc2l(2, hb.edge_len-1)])
  # lower right
  add_edge(hb.hex_graph, node=hb.rc2l(hb.edge_len, hb.edge_len),
            tonodes=[hb.rc2l(hb.edge_len, (hb.edge_len - 1)), hb.rc2l((hb.edge_len - 1), hb.edge_len)])
  # lower left
  add_edge(hb.hex_graph, node=hb.rc2l(hb.edge_len, 1), 
            tonodes=[hb.rc2l((hb.edge_len - 1), 1),hb.rc2l(hb.edge_len, 2),hb.rc2l((hb.edge_len - 1), 2)])

  # 4 borders (excluding corners)  4 edges per node.
  # north-south edges: constant row, vary col
  for c in 2..hb.edge_len-1:
    var r: int = 1
    add_edge(hb.hex_graph, node=hb.rc2l(r, c), 
              tonodes=[hb.rc2l(r, c - 1), hb.rc2l(r, c + 1), hb.rc2l(r + 1, c - 1), hb.rc2l(r + 1, c)])

    r = hb.edge_len
    add_edge(hb.hex_graph, node=hb.rc2l(r, c), 
            tonodes=[hb.rc2l(r, c - 1), hb.rc2l(r, c + 1), hb.rc2l(r - 1, c), hb.rc2l(r - 1, c + 1)])
    
  # east-west edges: constant col, vary row
  for r in 2..hb.edge_len-1:
    var c: int = 1
    add_edge(hb.hex_graph, node=hb.rc2l(r, c), 
            tonodes=[hb.rc2l(r - 1, c), hb.rc2l(r - 1, c + 1), hb.rc2l(r, c + 1), hb.rc2l(r + 1, c)])

    c = hb.edge_len
    add_edge(hb.hex_graph, node=hb.rc2l(r, c), 
          tonodes=[hb.rc2l(r - 1, c), hb.rc2l(r, c - 1), hb.rc2l(r + 1, c - 1), hb.rc2l(r + 1, c)])

  # interior tiles: 6 edges per hex
  for r in 2..hb.edge_len-1:
    for c in 2..hb.edge_len-1:
      add_edge(hb.hex_graph, node=hb.rc2l(r, c), tonodes=[hb.rc2l(r - 1, c + 1), hb.rc2l(r, c + 1),
                        hb.rc2l(r + 1, c), hb.rc2l(r + 1, c - 1), hb.rc2l(r, c - 1), hb.rc2l(r - 1, c)])


# helper to display_board: catenate marker at board position to the spacer between positions
proc markerdash(val: Marker, last: bool): string =
  var
    segment: string
    spacer = "___"
  let
    dot = "."
    x = "X"
    o = "O"

  if last:
    spacer = ""

  if val == Marker.empty:
    segment = dot & spacer
  elif val == Marker.playerX:
    segment = x & spacer
  elif val == Marker.playerO:
    segment = o & spacer
  else:
    raise newException(ValueError, "Error: invalid hexboard value.")

  return segment


# create ascii display of hexboard
proc display_board*(hb: Hexboard) =
  var 
    last: bool
  let
    connector:            string  = r" \ /"
    last_connector:       string = r" \"
  
  # number legend across the top of the board
  write(stdout, "  ", 1)
  for col in 2..hb.edge_len:
    if col < 10:
      write(stdout, "   ", col)
    else:
      write(stdout, "  ", col)
  echo() # just to get a newline

  # format two lines for each row, except the last row
  for row in 1..hb.edge_len:
    if row < 10:
      write(stdout, lead_space(row-1), row, " ")
    else:
      write(stdout, lead_space(row - 2), " ", row, " ")
    for col in 1..hb.edge_len:
      last = if col < hb.edge_len: false else: true
      write(stdout, markerdash(hb.get_hex_marker(row, col), last))
    echo()

    # connector lines to show edges between board positions
    if row != hb.edge_len:
      write(stdout, lead_space(row))
      write(stdout, repeat(connector, (hb.edge_len - 1)), last_connector, "\n")
    else:
      write(stdout, "\n\n")
  
# output move history to a "File" defaulting to stdout
proc display_move_history*(hb: Hexboard, mh: seq[Move], outf: File = stdout) =
  write(outf, "player  ","  linear", "   row", "   col\n" )
  for move in mh:
    write(outf,  $move.player,  fmt"{(hb.rc2l(move.row, move.col)):>7}",  
      fmt"{move.row:>8}",  fmt"{move.col:>6}", "\n")

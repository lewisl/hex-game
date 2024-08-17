
import 
  tables,
  strutils,
  graph

type 
  Marker*  = enum
    empty
    playerX
    playerO

type    
  Do_move* = enum
    naive, monte_carlo

type  
  RowCol* = object
    row*: int = 0
    col*: int = 0

type
  Hexboard* = object
    # used by hex_board
    hex_graph*:           Graph[Marker]
    edge_len*:            int
    max_idx*:             int
    move_count*:          int
    rand_nodes*:          seq[int]
    start_border*:        seq[seq[int]]
    finish_border*:       seq[seq[int]]
    # used by game_play  
    move_seq*:            seq[seq[RowCol]]
    win_pct_per_move*:    seq[float]
    neighbors*:           seq[int]
    captured*:            seq[int]
    shuffle_idxs*:        seq[int]
    empty_idxs*:          seq[int]
    winner_assess_time_t0*:  float
    winner_assess_time_cum*: float
    move_sim_time_t0*:       float
    move_sim_time_cum*:      float

  
proc newhexboard*(edge_len: int) : Hexboard =  # in c++ terms, a custom constructor
  var 
    edge_len = edge_len
    max_idx = edge_len * edge_len
  var hb = Hexboard(edge_len: edge_len, 
                    max_idx: max_idx,
                    move_seq: newSeq[newSeq[RowCol](max_idx div 2 + 1)](3),
                    start_border: newSeq[newSeq[int](edge_len)](3),
                    finish_border: newSeq[newSeq[int](edge_len)](3))
  hb.hex_graph = newgraph[Marker](empty, max_idx)

  for i in 0..max_idx-1:
    hb.rand_nodes.add(i)
  for i in 1..2:
    hb.move_seq[i].add(@[]) # add instantiated empty seq[RowCol]

  return hb

# conversions between row/col indices and ordinal integer indices to board positions
proc rowcol2linear(hb: Hexboard, row: int, col: int) : int  =
  let r = row - 1
  let c = col - 1

  if (r < hb.edge_len) and (c < hb.edge_len):
    return (r * hb.edge_len) + c
  else:
    raise newException(ValueError, "Bad row or col input: both must be >= edge length")

proc rowcol2linear*(hb: Hexboard, rc: RowCol) : int  =
  return hb.rowcol2linear(rc.row, rc.col)

proc linear2rowcol*(hb: Hexboard, linear: int) : RowCol  =
  if linear < hb.max_idx:
    return RowCol(row: (linear div hb.edge_len) + 1, col: (linear mod hb.edge_len) + 1)
  else:
    raise newException(ValueError, "Error: linear index input greater than edge_len * edge_len." )

proc is_empty*(hb: Hexboard, linear: int) : bool =
  return hb.hex_graph.node_data[linear] == Marker.empty

proc is_empty*(hb: Hexboard, rc: RowCol) : bool =
  return hb.hex_graph.node_data[hb.rowcol2linear(rc)] == Marker.empty


# getters and setters from hex_board to graph: maybe this is reason to shadow node_data
proc set_hex_marker*(hb: var Hex_board, hex_graph: var Graph, rc: RowCol, val: Marker)  = 
  hex_graph.set_node_data(hb.rowcol2linear(rc), val)
proc set_hex_marker*(hb: var Hex_board, hex_graph: var Graph, row: int, col: int, val: Marker)  =
  hex_graph.set_node_data(hb.rowcol2linear(row, col), val)
proc set_hex_marker*(hb: var Hex_board, hex_graph: var Graph, linear: int, val: Marker)  =
  hex_graph.set_node_data(linear, val)

proc get_hex_Marker*(hb: Hex_board, hex_graph: Graph[Marker], rc: RowCol) : Marker  =
  return get_node_data[Marker](hex_graph, hb.rowcol2linear(rc))
proc get_hex_Marker*(hb: Hex_board, hex_graph: Graph[Marker], row: int, col: int) : Marker  =
  return get_node_data[Marker](hex_graph, hb.rowcol2linear(row, col))
proc get_hex_Marker*(hex_graph: Graph[Marker], linear: int) : Marker  =
  return  get_node_data[Marker](hex_graph, linear)


# catenate marker at board position to the spacer between positions
proc markerdash(val: Marker, last: bool): string =
  var
    segment: string
    dot  = "."
    x = "X"
    o = "O"
    spacer = "___"

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


proc lead_space(row: int): string =
  return repeat(" ", row * 2)


# create lists of the indices of positions in the borders of the board
proc define_borders(hb: var Hexboard) =
    
  # top border
  for col in 1..hb.edge_len:
    var row: int = 1
    hb.start_border[ord(Marker.playerX)].add(hb.rowcol2linear(row, col))
  
  # bottom border
  for col in 1..hb.edge_len:
    let row = hb.edge_len
    hb.finish_border[ord(Marker.playerX)].add(hb.rowcol2linear(row, col))

  # left border
  for row in 1..hb.edge_len:
    let col = 1
    hb.start_border[ord(Marker.playerO)].add(hb.rowcol2linear(row, col))
  
  # right border
  for row in 1..hb.edge_len:
    let col = hb.edge_len
    hb.finish_border[ord(Marker.playerO)].add(hb.rowcol2linear(row, col))


# create graph of hexboard positions
proc make_hex_graph*(hb: var Hexboard) =

  hb.define_borders()

  # add graph edges for adjacent hexes based on the layout of a Hex game
  #    linear indices run from 0 at upper, left then across the row,
  #    then down 1 row at the left edge, and across, etc.
  # 
  # 4 corners of the board: 2 or 3 edges per node                            
  # upper left
  hb.hex_graph.add_edge(hb.rowcol2linear(1, 1), hb.rowcol2linear(2, 1))
  hb.hex_graph.add_edge(hb.rowcol2linear(1, 1), hb.rowcol2linear(1, 2))
  # upper right
  hb.hex_graph.add_edge(hb.rowcol2linear(1, hb.edge_len), hb.rowcol2linear(1, (hb.edge_len - 1)))
  hb.hex_graph.add_edge(hb.rowcol2linear(1, hb.edge_len), hb.rowcol2linear(2, hb.edge_len))
  hb.hex_graph.add_edge(hb.rowcol2linear(1, hb.edge_len), hb.rowcol2linear(2, (hb.edge_len - 1)))
  # lower right
  hb.hex_graph.add_edge(hb.rowcol2linear(hb.edge_len, hb.edge_len), hb.rowcol2linear(hb.edge_len, (hb.edge_len - 1)))
  hb.hex_graph.add_edge(hb.rowcol2linear(hb.edge_len, hb.edge_len), hb.rowcol2linear((hb.edge_len - 1), hb.edge_len))
  # lower left
  hb.hex_graph.add_edge(hb.rowcol2linear(hb.edge_len, 1), hb.rowcol2linear((hb.edge_len - 1), 1))
  hb.hex_graph.add_edge(hb.rowcol2linear(hb.edge_len, 1), hb.rowcol2linear(hb.edge_len, 2))
  hb.hex_graph.add_edge(hb.rowcol2linear(hb.edge_len, 1), hb.rowcol2linear((hb.edge_len - 1), 2))

  # 4 borders (excluding corners)  4 edges per node.
  # north-south edges: constant row, vary col
  for c in 2..hb.edge_len-1:
    var r: int = 1
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c - 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c + 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c - 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c))

    r = hb.edge_len
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c - 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c + 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c + 1))
    
  # east-west edges: constant col, vary row
  for r in 2..hb.edge_len-1:
    var c: int = 1
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c + 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c + 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c))

    c = hb.edge_len
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c - 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c - 1))
    hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c))
    
  # interior tiles: 6 edges per hex
  for r in 2..hb.edge_len-1:
    # for (int c = 2 c != edge_len ++c) {
    for c in 2..hb.edge_len-1:
      hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c + 1))
      hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c + 1))
      hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c))
      hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r + 1, c - 1))
      hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r, c - 1))
      hb.hex_graph.add_edge(hb.rowcol2linear(r, c), hb.rowcol2linear(r - 1, c))


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
      write(stdout, markerdash(hb.get_hex_marker(hb.hex_graph,row, col), last))
    echo()

    # connector lines to show edges between board positions
    if row != hb.edge_len:
      write(stdout, lead_space(row))
      write(stdout, repeat(connector, (hb.edge_len - 1)), last_connector, "\n")
    else:
      write(stdout, "\n\n")
  
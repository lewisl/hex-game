
import tables
import deques


type
  Edge* = object
    tonode*: int = 0
    cost*:   int = 0

proc `$`*(e: Edge): string =
  return ("to: " & $e.tonode & " cost: " & $e.cost)

type
  Graph*[T_data] = object  # object referred to as hex_graph from hex_board
    size*: int
    gmap*: Table[int, seq[Edge]]
    node_data*: seq[T_data]
    node_elem*: T_data  # might not need this

proc count_nodes*(gr: Graph) : int =
  return gr.gmap.len

proc newgraph*[T_data](node_elem: T_data, size: int): Graph[T_data] =
  var gr = Graph[T_data](size: size, node_elem: node_elem) 
  for i in 0..(size-1):
    gr.node_data.add(node_elem)
    gr.gmap[i] = @[] # create a node and empty seq[Edge]
  return gr


# TODO: handle missing nodes, although less likely given full initialization
proc add_edge*(hex_graph: var Graph, node: int, tonode: int, cost: int = 0, bidirectional: bool = false) =
  var
    no_match: bool = true

  # check if node already has tonode
  for val in hex_graph.gmap[node]:
    if val.tonode == tonode:
      no_match = false
  
  if no_match:
    hex_graph.gmap[node].add(Edge(tonode: tonode, cost: cost))
    if bidirectional:
      hex_graph.gmap[tonode].add(Edge(tonode: node, cost: cost))


proc set_node_data*[T_data](hex_graph: var Graph, idx: int, val: T_data) =
  hex_graph.node_data[idx] = val

proc get_node_data*[T_data](hex_graph: Graph[T_data], idx: int) : T_data  =
  return hex_graph.node_data[idx]

# get the neighbors of a node as a vector of edges
proc get_neighbors*[T_data](hex_graph: Graph[T_data], current_node: int) : seq[Edge]  =
  return hex_graph.gmap[current_node]

# get the neighbors whose data match to filter data
proc get_neighbors*[T_data](hex_graph: Graph[T_data], current_node: int, item_filter: T_data) : seq[Edge]  =
  
  var res: seq[Edge]
  for e in hex_graph.gmap[current_node]:
    if hex_graph.node_data[e.tonode] == item_filter:
      res.add(e)
  return res


# get the neighbors that match the select data and are not in the excluded nodes in a set, deque or vector
proc get_neighbors*[T_data, T_cont](hex_graph: Graph[T_data], current_node: int, item_filter: T_data, exclude: T_cont) : seq[Edge]  =
  var res: seq[Edge]
  for e in hex_graph.gmap[current_node]:
    if hex_graph.node_data[e.tonode] == item_filter and (not (contains(exclude, e.to_node))):
      res.add(e)
  return res


# get the neighbor_nodes as a vector of nodes instead of the edges
proc get_neighbor_nodes*[T_data](hex_graph: Graph[T_data], current_node: int, item_filter: T_data) : seq[int]  =
  var res : seq[int]
  for e in get_neighbors(hex_graph, current_node, item_filter):
      res.add(e.to_node)
  return res;


# get the neighbor_nodes as a vector of nodes instead of the edges
proc get_neighbor_nodes*[T_data, T_cont](hex_graph: Graph[T_data], current_node: int, item_filter: T_data, exclude: T_cont) : seq[int]  =
  var res: seq[int]
  for e in get_neighbors(hex_graph, current_node, item_filter, exclude):
      res.add(e.to_node)
  return res;

# TODO: this might not print out in the load_from_file format
proc display_graph*(gr: Graph) =
  for idx in 0..gr.size:
    echo("node: ", idx)
    echo("    data: ", gr.get_node_data(idx))
    echo("    edges:")
    for e in gr.get_neighbors(idx):
      echo("       to: ", e.tonode, " cost: ", e.cost)

# output graph to a "File" defaulting to stdout
proc display_graph*(gr: Graph, outf: File = stdout) =
  for idx in 0 ..< gr.size:
    write(outf, "node: " & $idx & "\n")
    write(outf, "    data: " & $gr.get_node_data(idx) & "\n")
    write(outf,"    edges:\n")
    for e in gr.get_neighbors(idx):
      write(outf, "       to: " & $e.tonode & " cost: " & $e.cost & "\n")

# create a file variable and pass it to display_graph
proc display_graph*(gr: Graph, filename: string) =
  var f = open(filename, fmWrite)
  gr.display_graph(f)



  
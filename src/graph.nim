import 
  tables

type
  Edge* = object
    tonode*: int = 0
    cost*:   int = 0

proc `$`*(e: Edge): string =  # overload $ string op for Edge
  return ("to: " & $e.tonode & " cost: " & $e.cost)


type
  Graph*[T_data] = object  # referred to as hex_graph from hex_board
    size*: int
    gmap*: seq[seq[Edge]]
    node_data*: ref seq[T_data]
    node_elem*: T_data  # initial fill value for graph


proc count_nodes*(gr: Graph) : int =  # reasonable thing to have, not used
  return gr.gmap.len


proc newgraph*[T_data](size: int, node_elem: T_data, ): Graph[T_data] =  # in c++ terms, a custom constructor
  var gr = Graph[T_data](size: size, node_elem: node_elem) 
  new(gr.node_data)
  gr.node_data[] = newSeq[T_data](size)
  gr.gmap = newSeq[seq[Edge]](size)
  # initialize node_data
  for i in 0 ..< size:
    gr.node_data[i] = node_elem
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
    # if bidirectional:
    #   hex_graph.gmap[tonode].add(Edge(tonode: node, cost: cost))


# varargs version enables adding many edges to a node in one call
proc add_edge*(hex_graph: var Graph, node: int, cost: int = 0, bidirectional: bool = false, tonodes: varargs[int]) =
  for tonode in tonodes:
    add_edge(hex_graph, node, tonode, cost, bidirectional)


proc set_node_data*[T_data](hex_graph: var Graph, idx: int, val: T_data) =
  hex_graph.node_data[idx] = val


proc get_node_data*[T_data](hex_graph: Graph[T_data], idx: int) : T_data  =
  return hex_graph.node_data[idx]

# get the neighbors of a node as a vector of edges
proc get_neighbors*[T_data](hex_graph: Graph[T_data], current_node: int) : seq[Edge]  =
  return hex_graph.gmap[current_node]


# get the neighbors whose data match to filter data
proc get_neighbors*[T_data](hex_graph: Graph[T_data], current_node: int, item_filter: T_data) : seq[Edge]  =
  
  result = newSeqOfCap[Edge](6)
  for e in hex_graph.gmap[current_node]:
    if hex_graph.node_data[e.tonode] == item_filter:
      result.add(e)


# get the neighbors that match the select data and are not in the excluded nodes in a set, deque or vector
proc get_neighbors*[T_data, T_cont](hex_graph: Graph[T_data], current_node: int, item_filter: T_data, exclude: T_cont) : seq[Edge]  =
  result = newSeqOfCap[Edge](6)
  for e in hex_graph.gmap[current_node]:
    if hex_graph.node_data[e.tonode] == item_filter and (not (contains(exclude, e.to_node))):
      result.add(e)
  

# get the neighbor_nodes as a vector of nodes instead of the edges
proc get_neighbor_nodes*[T_data](hex_graph: Graph[T_data], current_node: int, item_filter: T_data) : seq[int]  =
  result = newSeqOfCap[int](6)
  for e in get_neighbors(hex_graph, current_node, item_filter):
      result.add(e.to_node)


# get the neighbor_nodes as a vector of nodes instead of the edges
proc get_neighbor_nodes*[T_data, T_cont](hex_graph: Graph[T_data], current_node: int, 
    item_filter: T_data, exclude: T_cont) : seq[int]  =
  result = newSeqOfCap[int](6)
  for e in get_neighbors(hex_graph, current_node, item_filter, exclude):
      result.add(e.to_node)


# output graph to a "File" defaulting to stdout
proc display_graph*(gr: Graph, outf: File = stdout) =
  for idx in 0 ..< gr.size:
    write(outf, "node: " & $idx & "\n")
    write(outf, "    data: " & $gr.get_node_data(idx) & "\n")
    write(outf,"    edges:\n")
    for e in gr.get_neighbors(idx):
      write(outf, "       to: " & $e.tonode & " cost: " & $e.cost & "\n")

# create a file variable and pass it to display_graph to save graph in a text file
proc display_graph*(gr: Graph, filename: string) =
  var f = open(filename, fmWrite)
  gr.display_graph(f)
  f.close

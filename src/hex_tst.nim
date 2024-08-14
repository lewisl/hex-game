import hex_board
import graph
import helpers

var hb: Hexboard = newhexboard(7)
hb.make_board()
echo("count nodes: ",hb.hex_graph.count_nodes)

# hb.hex_graph.display_graph() # works to first approximation

var testnode = 12

echo(hb.hex_graph.get_neighbors(testnode)) # @[to: 7 cost: 0, to: 1 cost:0]
echo(hb.hex_graph.get_neighbors(testnode, Marker.playerO))  # @[]

# var 
#   rc: RowCol
# rc = RowCol(row: 1, col: 1)
 
# echo(rc)

var sq = @[1,2,3,4,5,9]
echo(contains(sq, 11))




hb.set_hex_marker(hb.hex_graph, 5, playerO)
hb.set_hex_marker(hb.hex_graph, 6, playerO)
hb.set_hex_marker(hb.hex_graph, 13, playerO)
hb.set_hex_marker(hb.hex_graph, 19, playerX)
hb.set_hex_marker(hb.hex_graph, 18, playerX)

echo("\nneighbors of ", testnode, " that contain ", Marker.playerO)
echo(hb.hex_graph.get_neighbors(testnode, Marker.playerO))  # @[to: 1, cost: 0] passed
echo(hb.hex_graph.get_neighbor_nodes(testnode, Marker.playerO)) # @[6, 13, 5] passed

echo("\nneighbors of ", testnode, " that contain ", Marker.playerX)
echo(hb.hex_graph.get_neighbors(testnode, Marker.playerX))  # @[to: 1, cost: 0] passed
echo(hb.hex_graph.get_neighbor_nodes(testnode, Marker.playerX)) # @[19, 8] passed

echo("\nusing exclude of specific nodes")
echo(hb.hex_graph.get_neighbors(testnode, Marker.playerO, @[5,6]))  # @[to: 1, cost: 0] passed
echo(hb.hex_graph.get_neighbor_nodes(testnode, Marker.playerO, @[5,6])) # @[1] passed

# hb.display_board()  # passes!




(hb.hex_graph).display_graph("foo.txt")

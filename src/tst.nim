import tables
import sequtils
import times
import random
import strutils
import system/iterators
import std/os

var t0 = cpuTime()

type 
  Marker*  = enum
    empty
    playerX
    playerO

type
  Edge* = object
    tonode: int = 0
    cost:   int = 0
type
  RowCol = object
    row: int
    col: int

proc `$`(e: Edge): string =
  return ("to: " & $e.tonode & " cost: " & $e.cost)

type
  Graph*[T_data] = object
    graph*: Table[int, seq[Edge]]
    node_data*: seq[T_data]
    node_elem*: T_data  # might not need this



type
  Play* = object
    start_border*:    seq[seq[int]] 
    move_seq:         seq[seq[Edge]]
    node_data:        seq[int]

type
  Shadow = object
    p_move_seq:       ptr seq[seq[Edge]]

var
  sh = Shadow()

var
  pl = Play()

sh.p_move_seq = addr(pl.move_seq)


echo(pl)
for i in 0..2:
  pl.move_seq.add(@[])

for i in 0..2:
  pl.move_seq[i].add(Edge())
  pl.move_seq[i].add(Edge())

echo(pl.move_seq)
echo(pl.move_seq[0])
echo(sh.p_move_seq[0])

sh.p_move_seq[0][1] = Edge(tonode: 5, cost: 2)

echo(pl.move_seq[0])
echo(sh.p_move_seq[0])

# proc tryseqs(pl: Play) =
#   pl.move_seq.add(seq[Edge]()) # create outer vector; add inner vector containing default Edge
#   # pl.move_seq[0].add(Edge(tonode: 5,cost: 5)) # add another edge to the inner vec at idx 0
#   # pl.move_seq.add(@[]) # add an empty vec into the outer vec at idx 1
#   # pl.move_seq[1].add(Edge(tonode: 3, cost: 4))  # add an Edge
    
#   echo("\nouter vector at pl.move_seq: ", pl.move_seq)
#   # echo("pl.move_seq[0]: ", pl.move_seq[0])
  # echo("pl.move_seq[1]: ", pl.move_seq[1])

proc trytable() =  # try the table and generic for the object graph
  var
    gr: Graph[Marker]

  gr.graph[0] = @[Edge(tonode: 2, cost: 0)] # add key 0, value vector of one edge
  gr.graph[1] = @[Edge(tonode: 0)]
  gr.node_data.add(Marker.empty)
  gr.node_data.add(empty)

  for val in values(gr.graph):
    echo(val)

  # echo("\n\n")
  # echo("All of the graph object gr: ",gr)
  # echo("zeroth element of gr.node_data: ", gr.node_data[0])

# var foo = newSeq[int](5)
# echo foo

# import std/strformat
# let msg = "hello"

# echo fmt"{msg}{'\n'}"
# echo fmt("{msg}\n")
# echo "{msg}\n".fmt
# echo &"{msg}\n"

# var fx : float64=45.56

# echo "$fx is a float64"  

var edg:  Edge = Edge(tonode: 5, cost: 4)

# var tstseq = @[0, 6, 7, 9, 11]
# echo(sequtils.any(tstseq, proc (x: int): bool = x == 6))


var s = newSeqWith(8,0)
echo(s)
var s_of_s = newSeqWith(5,newSeq[Edge](3))
echo(s_of_s)

# shuffling
echo("SHUFFLING")
# var sr = newSeqWith(10, (it, inc))
var sr = (0..1_000).toSeq
echo(sr[10..13])
var randgen = initRand(101)
randgen.shuffle(sr)
echo(sr[10..13])

echo("total time: ", cpuTime()-t0)
# avoid newline that echo always adds.
# write(stdout, edg)
# echo()
# write(stdout, "foo")
# echo()

# echo(string_by_n("j ", 5))

# this works!
# proc move_input(msg: string) : RowCol =
#   var 
#     row, col: int
#     input: string
#     items: seq[string]

#   while true:
#     input = readLine(stdin)
#     items = input.splitWhitespace()
#     if items.len != 2:
#       write(stdout, msg)
#       continue
#     else:
#       row = items[0].parseInt
#       col = items[1].parseInt
#       break
  
#   return RowCol(row: row, col: col)

# write(stdout, "Enter 2 integers: 1 2")
# discard move_input("enter 2 integers: ")

var date = now()
var filename = "board graph " & $date & ".txt"
echo(filename)

var ctr = 1
ctr.inc()
echo(ctr)

echo("remainder: ", $(5 mod 3))

var f = open("foo", fmReadWrite)

echo("type of fstream ", typeof(f))

echo("type of stdout ", typeof(stdout))

var sq = @[1, 4, 6, 9]
echo("Last element of sq ", sq[sq.high])

var fv = 5
var fr = 4

echo(fv in sq)
echo(fr in sq)

let a = 2

for i in countup(a, a + 12, a + 1):  # very tricky because the loop counter is i, so the increment is 3
  echo i

# echo(contains(sq, fv))
# echo(contains(sq,fr))
# echo(contains("yes", "y"))


# echo(repeat("foo ", 3))

# write(stdout, repeat("\n", 15))

# proc clear_screen*() =
#   discard execShellCmd("clear")

# clear_screen()
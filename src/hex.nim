# nim -d:release --outdir:build c  src/hex.nim

import hex_board
import game_play
import os
import strutils
import random


var size: int = 5
var n_trials: int = 1000

randomize()

echo("paramcount: ", $paramCount())

if paramCount() == 0:
  discard # run with defaults
elif paramCount() == 1:
  size = parseInt(paramStr(1))
elif paramCount() == 2:
  size = parseInt(paramStr(1)) 
  n_trials = parseInt(paramStr(2))
else:    
  echo("Wrong number of input arguments.")
  echo("Run as hex [size] [n_trials]. Exiting...")
  quit()

if (size < 0) or (size mod 2) == 0:
  quit("Bad size input. Must be odd, positive integer. Exiting...")

var hb = newhexboard(size)
hb.make_board()

hb.play_game(Do_move.monte_carlo, n_trials)

echo "Assessing who won took " , hb.winner_assess_time_cum, " seconds.\n";
echo("Simulating and evaluating moves took ", 
  hb.move_sim_time_cum - hb.winner_assess_time_cum, " seconds.\n");

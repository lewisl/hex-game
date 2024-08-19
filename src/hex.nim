# nim c -d:release --mm:orc --threads:on --opt:speed --outdir:nim_build src/hex.nim

#######################################################
# script to initiate the game
#######################################################

import 
  os,
  strutils,
  hex_board,
  game_play

proc main() =
  var 
    size: int = 5
    n_trials: int = 1000

  let 
      pcnt: int = paramCount()
      
  if pcnt == 0:
    discard  # do nothing and run with defaults
  elif pcnt == 1:
    size = parseInt(paramStr(1))
  elif pcnt == 2:
    size = parseInt(paramStr(1)) 
    n_trials = parseInt(paramStr(2))
  else:    
    echo("Wrong number of input arguments.")
    echo("Run as hex [size] [n_trials]. Exiting...")
    quit()

  if (size < 0) or (size mod 2) == 0:
    quit("Bad size input. Must be odd, positive integer. Exiting...")

  var hb = newhexboard(size) # create the game object
  hb.make_hex_graph()

  hb.play_game(Do_move.monte_carlo, n_trials)

  echo "Assessing who won took " , hb.winner_assess_time_cum, " seconds.";
  echo("Simulating and evaluating moves took ", 
    hb.move_sim_time_cum - hb.winner_assess_time_cum, " seconds.\n");

main()

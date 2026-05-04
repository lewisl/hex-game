# nim c -d:release --mm:arc --opt:speed -d:lto --outdir:nim_build nim-src/hex.nim

#######################################################
# script to initiate the game
#######################################################

import 
  os,
  strutils,
  # project modules
  hex_board,
  game_play

proc main() =
  var 
    size: int = 5
    n_trials: int = 1500
    debug: bool = false

  let 
      pcnt: int = paramCount()
      
  if pcnt == 0:
    discard  # do nothing and run with defaults
  elif pcnt == 1:
    size = parseInt(paramStr(1))
  elif pcnt == 2:
    size = parseInt(paramStr(1)) 
    n_trials = parseInt(paramStr(2))
    if (n_trials < 250):
      echo("Number of trials input, 2nd parameter, must be between 250 and 10000:")
      echo("Proceeding with n_trials = 250")
      n_trials = 250
  elif pcnt == 3:
    size = parseInt(paramStr(1)) 
    n_trials = parseInt(paramStr(2))    
    debug = parseBool(paramstr(3))
  else:    
    echo "Wrong number of input arguments."
    echo "Run as hex [size] [n_trials] [debug] Exiting..."
    echo "To enter the 2nd or 3rd arguments, you must also enter the preceding arguments."
    quit()

  if (size < 2 or size > 21) : 
    quit("Bad size input. Must be positive integer between 2 and 21. Exiting...")

  var hb = initHexboard(size) # create the game object
  hb.make_hex_graph()

  hb.play_game(n_trials, debug)

  echo("Simulating and evaluating moves took ", 
    hb.move_sim_time_cum, " seconds.\n");

  if debug:
    hb.save_diagnostics()

when isMainModule:
  main()

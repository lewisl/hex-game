import std/os

#[
This works great, but nim has contains so unneeded...
# simple linear search through small containers
proc is_in*[T_cont, T](val: T, cont: T_cont) : bool =
  var found: bool = false
  for item in cont:
    if item == val:
      found = true
      break
  return found
  ]#

#[ This works but nim has repeat, so not needed...
# TODO replace with repeat from strutils
# repeat a string n times into a new string
proc string_by_n*(s: string, n: int) : string =
  var ret: string
  for i in 0..n-1:
    ret &= s
  return ret
]#
  

# clear screen not guaranteed to work on all OS'es.  Works on MacOs.
# use "cls" for Windows
proc clear_screen*() =
  # write(stdout, u8"\033[2J")
  discard execShellCmd("clear")
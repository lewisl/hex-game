add_rules("mode.release")

target("hex") 
    set_kind("binary")
    add_files("src/hex.cpp", "src/game_play.cpp", "src/hex_board.cpp", "src/hex_board.cpp")
    set_languages("cxx17")
    set_optimize("fastest")
    add_cxxflags("-flto")  -- supposed to be linker optimization; doesn't really do much

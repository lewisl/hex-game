target("hex") 
    set_kind("binary")
    add_files("src/hex.cpp", "src/game_play.cpp", "src/hex_board.cpp", "src/hex_board.cpp")
    -- add_includedirs("src")
    set_languages("cxx14")
    set_optimize("fastest")





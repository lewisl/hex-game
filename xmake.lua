add_rules("mode.release")

target("hexcpp") 
    set_kind("binary")
    add_files("cpp-src/hex.cpp", "cpp-src/game_play.cpp", "cpp-src/hex_board.cpp", "cpp-src/hex_board.cpp")
    set_languages("cxx17")
    set_optimize("fastest")
    -- add_cxxflags("-flto")  -- supposed to be linker optimization; doesn't really do much
            -- these don't work... -fprofile-instr-generate and -fprofile-instr-use

target("hexnim") 
    set_kind("binary")
    add_files("nim-src/*.nim")
    set_optimize("fastest")
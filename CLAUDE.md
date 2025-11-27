# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This repository contains dual implementations of the Hex board game in both C++ and Nim. The implementations use Monte Carlo simulation to play against human opponents. The C++ version is object-oriented (based on classes), while the Nim version is procedural (using structs with uniform function call syntax).

## Build and Run Commands

### C++ Version
```bash
# Build the C++ executable
xmake build hexcpp

# Run the C++ version (default: 5x5 board, 1000 trials)
./build/macosx/arm64/release/hexcpp
```

### Nim Version
```bash
# Build the Nim executable
xmake build hexnim

# Run the Nim version (default: 5x5 board, 1500 trials)
./build/macosx/arm64/release/hexnim

# Run with custom parameters: size, n_trials, debug mode
./build/macosx/arm64/release/hexnim 7 2000 false
```

### Build System
- Uses `xmake` as the build system
- Build configuration is in [xmake.lua](xmake.lua)
- Both targets compile with optimization flags enabled (`release` mode, `fastest` optimization)

## Code Architecture

### High-Level Design

Both implementations share the same conceptual architecture:

1. **Graph Representation**: The Hex board is represented as a graph where each hexagon is a node, and edges connect adjacent hexagons
2. **Game Play Logic**: Uses Monte Carlo simulation to evaluate moves - for each potential move, simulate random game completions and count wins
3. **Winner Detection**: Depth-first search to find connected paths from start border to finish border

### Core Components

#### C++ Implementation ([cpp-src/](cpp-src/))

**Graph Class** ([graph.h](cpp-src/graph.h))
- Generic template class `Graph<T_data>` for graph operations
- Stores node data (Marker values) and edges (adjacency lists)
- Methods: `get_neighbors()`, `get_neighbor_nodes()`, `add_edge()`, `load_graph_from_file()`

**Hex Class** ([hex.h](cpp-src/hex.h), [hex_board.cpp](cpp-src/hex_board.cpp), [game_play.cpp](cpp-src/game_play.cpp))
- Contains a `Graph<Marker>` member using composition
- Key nested types:
  - `Marker` enum: empty, playerX, playerO
  - `RowCol` struct: board position (1-based indexing for users)
  - `Move` struct: records player, row, col
- Index conversion: `rc2l()` (row/col to linear), `l2rc()` (linear to row/col)
- Board creation: `make_board()` creates graph edges for hex adjacency
- Game play: `play_game()`, `monte_carlo_move()`, `find_ends()`
- Pre-allocated memory: `empty_idxs`, `shuffle_idxs`, `wins_per_move` to reduce allocations during simulation

#### Nim Implementation ([nim-src/](nim-src/))

**graph.nim** ([nim-src/graph.nim](nim-src/graph.nim))
- Generic `Graph[T_data]` type with same functionality as C++ version
- Uses `seq[seq[Edge]]` for adjacency lists
- `node_data` is a traced reference (`ref seq[T_data]`)

**hex_board.nim** ([nim-src/hex_board.nim](nim-src/hex_board.nim))
- `Hexboard` object containing graph and game state
- Same types: `Marker`, `RowCol`, `Move`
- `positions` field is a traced reference aliasing `hex_graph.node_data`
- Board display: `display_board()`, `display_move_history()`

**game_play.nim** ([nim-src/game_play.nim](nim-src/game_play.nim))
- Game logic: `play_game()`, `monte_carlo_move()`, `find_ends()`
- Uses Nim's uniform function call syntax: `hb.play_game()` passes `hb` as first parameter
- `simulate_hexboard_positions()` fills board with random moves
- Winner detection uses depth-first search with `Deque[int]` for candidates

**hex.nim** ([nim-src/hex.nim](nim-src/hex.nim))
- Entry point with command-line argument parsing
- Creates `Hexboard` with `initHexboard()`, calls `make_hex_graph()` and `play_game()`

### Key Architectural Details

**Index Conversion**
- Users see 1-based row/col indices
- Internal linear indices are 0-based
- All conversions happen through `rc2l()` and `l2rc()` methods

**Monte Carlo Algorithm**
- For each empty position, place test move
- Run N trials: randomly fill remaining positions, check if current player wins
- Track wins per test move
- Choose move with highest win percentage
- Reset board state after evaluation

**Winner Detection**
- PlayerX wins by connecting top to bottom
- PlayerO wins by connecting left to right
- `find_ends()` uses depth-first search starting from finish border
- Searches backward through connected same-colored hexagons to start border
- Uses `captured` vector/seq to track visited nodes, `possibles` deque for DFS frontier

**Performance Optimizations**
- Pre-allocate vectors/seqs for simulation to avoid repeated allocations
- Use fast random number generators (minstd_rand in C++, Nim's random module)
- Reuse shuffle_idxs between moves with single-element swaps
- Reference sharing in Nim: `positions` references `hex_graph.node_data` to avoid copies

## Code Organization Patterns

### C++
- Header file ([hex.h](cpp-src/hex.h)) contains class definition
- Implementation split across [hex_board.cpp](cpp-src/hex_board.cpp) (board creation/display) and [game_play.cpp](cpp-src/game_play.cpp) (game logic)
- Uses class methods with private/public access

### Nim
- Each module imports dependencies at top
- Uses object types with public fields (marked with `*`)
- Procedures use UFCS: first parameter is the object being operated on
- Forward declarations used for circular dependencies

## Testing

- Test file exists at [tests/hex_tests.cpp](tests/hex_tests.cpp) but is not integrated into build
- No automated test harness currently configured
- Manual testing through gameplay

## Performance Notes

From the README:
- Nim version runs ~20% faster with simple RNGs
- Nim executable is 2x larger than C++ executable
- Nim uses ~25% more memory at high water mark
- Both are optimized for release builds with LTO enabled

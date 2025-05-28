# Simple Chess Engine

A basic but well-structured UCI-compatible chess engine written in C++. This engine is designed to be easily upgradable and serves as a foundation for more advanced features.

## Features

- UCI (Universal Chess Interface) protocol support
- Basic board representation with piece-square arrays
- Move generation for all piece types
- Position evaluation with material and piece-square tables
- Alpha-beta search with quiescence search
- Simple transposition table
- FEN string support

## Building

### Requirements
- C++17 compatible compiler (g++ or clang++)
- Make build system

### Compilation
```bash
make
```

For debug build:
```bash
make debug
```

For optimized build with profiling:
```bash
make profile
```

## Usage

### Running the Engine
```bash
./chess_engine
```

### UCI Commands
The engine supports standard UCI commands:

- `uci` - Initialize UCI mode
- `isready` - Check if engine is ready
- `ucinewgame` - Start a new game
- `position [startpos | fen <fenstring>] [moves <move1> <move2> ...]` - Set position
- `go [depth <x>] [movetime <ms>] [wtime <ms>] [btime <ms>] [infinite]` - Start calculating
- `stop` - Stop calculating
- `quit` - Exit the engine

### Example Session
```
./chess_engine
uci
id name SimpleChessEngine
id author YourName
uciok
isready
readyok
position startpos
go depth 5
info depth 1 score cp 20 nodes 21 time 1 nps 21000 pv e2e4
info depth 2 score cp 0 nodes 84 time 2 nps 42000 pv e2e4 e7e5
info depth 3 score cp 25 nodes 564 time 5 nps 112800 pv e2e4 e7e5 g1f3
info depth 4 score cp 0 nodes 2314 time 15 nps 154266 pv e2e4 e7e5 g1f3 b8c6
info depth 5 score cp 30 nodes 11523 time 73 nps 157876 pv e2e4 e7e5 g1f3 b8c6 f1b5
bestmove e2e4
quit
```

## Architecture

### Core Components

1. **Board Representation** (`board.h/cpp`)
   - 8x8 array representation
   - Zobrist hashing for position identification
   - Move/unmove with full state restoration
   - FEN parsing and generation

2. **Move Generation** (`movegen.h/cpp`)
   - Generates pseudo-legal moves
   - Separate generators for each piece type
   - Capture-only generation for quiescence search

3. **Evaluation** (`evaluation.h/cpp`)
   - Material counting
   - Piece-square tables
   - Basic pawn structure evaluation
   - King safety considerations

4. **Search** (`search.h/cpp`)
   - Alpha-beta pruning
   - Quiescence search
   - Simple transposition table
   - Iterative deepening

5. **UCI Interface** (`uci.h/cpp`)
   - Full UCI protocol implementation
   - Time management
   - Multi-threaded search support

## Extending the Engine

The modular design makes it easy to add features:

### 1. Improved Evaluation
- Add mobility evaluation
- Implement pawn structure analysis
- Add endgame-specific knowledge

### 2. Search Enhancements
- Implement null move pruning
- Add history heuristic
- Implement late move reductions (LMR)
- Add aspiration windows

### 3. Move Ordering
- Implement killer moves
- Add history heuristic
- Improve MVV-LVA implementation

### 4. Opening Book
- Add opening book support
- Implement book learning

### 5. Endgame Tablebases
- Add Syzygy tablebase support
- Implement basic endgame knowledge

## Performance Tips

1. **Optimization Flags**: The Makefile uses `-O3 -march=native` for maximum performance
2. **Transposition Table**: Increase size for deeper searches
3. **Move Ordering**: Better move ordering dramatically improves search efficiency
4. **Evaluation Caching**: Cache expensive evaluation components

## Testing

### Perft Testing
Implement perft (performance test) to verify move generation:
```cpp
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    uint64_t nodes = 0;
    std::vector<Move> moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    for (Move m : moves) {
        board.makeMove(m);
        nodes += perft(board, depth - 1);
        board.unmakeMove(m);
    }
    
    return nodes;
}
```

### Engine Testing
- Test against other engines using Arena or Cute Chess
- Use test suites (WAC, ECM, etc.)
- Implement self-play for testing improvements

## Known Limitations

This is a basic implementation. Key areas for improvement:
- Move generation could use bitboards for efficiency
- Evaluation is very simple
- Search lacks many standard optimizations
- No opening book or endgame tablebases

## License

This project is provided as-is for educational purposes. Feel free to use and modify as needed.

## Contributing

Contributions are welcome! Some areas that need work:
- Complete implementation of move generation
- Improve evaluation function
- Add more search optimizations
- Implement missing UCI features
- Add comprehensive testing
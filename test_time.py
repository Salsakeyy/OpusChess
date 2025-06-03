#!/usr/bin/env python3
"""Test time management of the chess engine"""

import subprocess
import time

def test_time_management():
    # Start the engine
    engine = subprocess.Popen(
        ["./chess_engine"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
        bufsize=1
    )
    
    # Initialize UCI
    engine.stdin.write("uci\n")
    engine.stdin.flush()
    
    # Read until uciok
    while True:
        line = engine.stdout.readline().strip()
        print(f"Engine: {line}")
        if "uciok" in line:
            break
    
    # Send isready
    engine.stdin.write("isready\n")
    engine.stdin.flush()
    
    # Read until readyok
    while True:
        line = engine.stdout.readline().strip()
        print(f"Engine: {line}")
        if "readyok" in line:
            break
    
    # Test 1: Fixed time test
    print("\n=== Test 1: 1000ms time limit ===")
    engine.stdin.write("position startpos\n")
    engine.stdin.flush()
    
    start_time = time.time()
    engine.stdin.write("go movetime 1000\n")
    engine.stdin.flush()
    
    # Read output until bestmove
    depths_reached = []
    while True:
        line = engine.stdout.readline().strip()
        print(f"Engine: {line}")
        if "info depth" in line:
            parts = line.split()
            depth_idx = parts.index("depth") + 1
            if depth_idx < len(parts):
                depths_reached.append(int(parts[depth_idx]))
        if "bestmove" in line:
            break
    
    elapsed = time.time() - start_time
    print(f"Time taken: {elapsed:.3f}s (expected ~1.0s)")
    print(f"Depths reached: {depths_reached}")
    
    # Test 2: Time control test
    print("\n=== Test 2: Time control (wtime 5000 btime 5000 winc 100 binc 100) ===")
    engine.stdin.write("position startpos\n")
    engine.stdin.flush()
    
    start_time = time.time()
    engine.stdin.write("go wtime 5000 btime 5000 winc 100 binc 100\n")
    engine.stdin.flush()
    
    # Read stderr to see time management debug info
    # Read output until bestmove
    depths_reached = []
    allocated_time = None
    while True:
        # Check stderr for debug output
        try:
            err_line = engine.stderr.readline()
            if err_line:
                print(f"Debug: {err_line.strip()}")
                if "timeLimit=" in err_line:
                    # Extract the time limit
                    parts = err_line.split("timeLimit=")
                    if len(parts) > 1:
                        time_str = parts[1].split("ms")[0]
                        allocated_time = int(time_str)
        except:
            pass
            
        line = engine.stdout.readline().strip()
        print(f"Engine: {line}")
        if "info depth" in line:
            parts = line.split()
            depth_idx = parts.index("depth") + 1
            if depth_idx < len(parts):
                depths_reached.append(int(parts[depth_idx]))
        if "bestmove" in line:
            break
    
    elapsed = time.time() - start_time
    print(f"Time taken: {elapsed:.3f}s")
    print(f"Allocated time: {allocated_time}ms" if allocated_time else "Unknown")
    print(f"Depths reached: {depths_reached}")
    
    # Quit
    engine.stdin.write("quit\n")
    engine.stdin.flush()
    engine.wait()

if __name__ == "__main__":
    test_time_management()
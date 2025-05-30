#!/usr/bin/env python3
"""
Simple Chess Engine Tester - No external dependencies
Directly interfaces with UCI engines to run tests
"""

import subprocess
import threading
import time
import math
import json
import os
import sys
import random
from datetime import datetime
from collections import defaultdict
import queue

class UCIEngine:
    """Simple UCI engine interface"""
    
    def __init__(self, engine_path):
        self.engine_path = engine_path
        self.process = None
        
    def start(self):
        """Start the engine process"""
        self.process = subprocess.Popen(
            [self.engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1
        )
        
    def send(self, command):
        """Send command to engine"""
        if self.process and self.process.poll() is None:
            try:
                self.process.stdin.write(command + '\n')
                self.process.stdin.flush()
            except (BrokenPipeError, OSError):
                # Process has terminated
                pass
            
    def receive_until(self, expected):
        """Receive output until expected string"""
        lines = []
        while self.process and self.process.poll() is None:
            try:
                line = self.process.stdout.readline().strip()
                if line:
                    lines.append(line)
                if expected in line:
                    break
            except:
                break
        return lines
        
    def quit(self):
        """Quit the engine"""
        if self.process:
            try:
                if self.process.poll() is None:  # Process still running
                    self.send('quit')
                    self.process.wait(timeout=5)
            except:
                pass
            finally:
                if self.process.poll() is None:
                    self.process.terminate()
                    self.process.wait(timeout=1)
                self.process = None

class GameManager:
    """Manages games between two engines"""
    
    # Common opening positions (first few moves)
    OPENINGS = [
        "position startpos",
        "position startpos moves e2e4",
        "position startpos moves e2e4 e7e5",
        "position startpos moves d2d4",
        "position startpos moves d2d4 d7d5",
        "position startpos moves e2e4 c7c5",
        "position startpos moves d2d4 g8f6",
        "position startpos moves e2e4 e7e6",
        "position startpos moves g1f3",
        "position startpos moves c2c4",
    ]
    
    def __init__(self, engine1_path, engine2_path, time_ms=10000, inc_ms=100):
        self.engine1_path = engine1_path
        self.engine2_path = engine2_path
        self.initial_time_ms = time_ms
        self.inc_ms = inc_ms
        
    def parse_move(self, response_lines):
        """Extract move from engine response"""
        for line in response_lines:
            if line.startswith('bestmove'):
                parts = line.split()
                if len(parts) > 1:
                    return parts[1]
        return None
        
    def check_game_end(self, moves):
        """Simple game end detection based on move patterns"""
        if len(moves) < 10:
            return False, None
            
        # Check for repetition (simplified - just check if last 8 moves repeat)
        if len(moves) >= 8:
            if moves[-4:] == moves[-8:-4]:
                return True, "draw"
                
        # Check for very long games
        if len(moves) > 200:
            return True, "draw"
            
        # No automatic end detection - games will be adjudicated
        return False, None
        
    def play_game(self, engine1_white=True):
        """Play a single game between engines"""
        engine1 = UCIEngine(self.engine1_path)
        engine2 = UCIEngine(self.engine2_path)
        
        try:
            # Start engines
            engine1.start()
            engine2.start()
            
            # Initialize
            engine1.send('uci')
            engine2.send('uci')
            engine1.receive_until('uciok')
            engine2.receive_until('uciok')
            
            # New game
            engine1.send('ucinewgame')
            engine2.send('ucinewgame')
            engine1.send('isready')
            engine2.send('isready')
            engine1.receive_until('readyok')
            engine2.receive_until('readyok')
            
            # Random opening
            position = random.choice(self.OPENINGS)
            moves = []
            if "moves" in position:
                moves = position.split("moves")[1].strip().split()
                
            # Track game state
            white_engine = engine1 if engine1_white else engine2
            black_engine = engine2 if engine1_white else engine1
            current_engine = white_engine
            
            # Time tracking
            white_time = self.initial_time_ms
            black_time = self.initial_time_ms
            
            # Play game
            max_moves = 200
            
            while len(moves) < max_moves:
                # Send position
                pos_cmd = "position startpos"
                if moves:
                    pos_cmd += " moves " + " ".join(moves)
                current_engine.send(pos_cmd)
                
                # Prepare go command with time management
                if current_engine == white_engine:
                    go_cmd = f'go wtime {white_time} btime {black_time} winc {self.inc_ms} binc {self.inc_ms}'
                else:
                    go_cmd = f'go wtime {white_time} btime {black_time} winc {self.inc_ms} binc {self.inc_ms}'
                
                # Send go command and track time
                start_time = time.time()
                current_engine.send(go_cmd)
                response = current_engine.receive_until('bestmove')
                elapsed_ms = int((time.time() - start_time) * 1000)
                
                # Update time
                if current_engine == white_engine:
                    white_time = max(100, white_time - elapsed_ms + self.inc_ms)
                else:
                    black_time = max(100, black_time - elapsed_ms + self.inc_ms)
                
                # Extract move
                move = self.parse_move(response)
                if not move or move == '(none)' or move == '0000':
                    # No legal moves or error
                    if current_engine == white_engine:
                        return 0.0 if engine1_white else 1.0  # White loses
                    else:
                        return 1.0 if engine1_white else 0.0  # Black loses
                    
                moves.append(move)
                
                # Check for game end
                ended, result = self.check_game_end(moves)
                if ended:
                    if result == "draw":
                        return 0.5
                    # Other end conditions would be handled here
                    
                # Check for time forfeit
                if (current_engine == white_engine and white_time < 100) or \
                   (current_engine == black_engine and black_time < 100):
                    # Time forfeit
                    if current_engine == white_engine:
                        return 0.0 if engine1_white else 1.0  # White loses on time
                    else:
                        return 1.0 if engine1_white else 0.0  # Black loses on time
                
                # Switch engines
                current_engine = black_engine if current_engine == white_engine else white_engine
                
            # Game too long - adjudicate as draw
            return 0.5
            
        except Exception as e:
            print(f"Error in game: {e}")
            return 0.5  # Draw on error
            
        finally:
            engine1.quit()
            engine2.quit()

class SimpleTester:
    """Simplified testing framework"""
    
    def __init__(self, base_engine, test_engine, **kwargs):
        self.base_engine = os.path.abspath(base_engine)
        self.test_engine = os.path.abspath(test_engine)
        
        # Test parameters
        self.num_games = kwargs.get('num_games', 100)
        self.concurrency = kwargs.get('concurrency', 1)
        self.time_ms = kwargs.get('time_ms', 10000)
        self.inc_ms = kwargs.get('inc_ms', 100)
        
        # Results
        self.wins = 0
        self.draws = 0
        self.losses = 0
        self.time_losses = 0
        self.lock = threading.Lock()
        
        # Progress
        self.games_completed = 0
        self.start_time = None
        
    def run_games(self, game_indices):
        """Worker function to run games"""
        manager = GameManager(self.test_engine, self.base_engine, self.time_ms, self.inc_ms)
        
        for i in game_indices:
            try:
                # Play pair of games with reversed colors
                result1 = manager.play_game(engine1_white=True)
                result2 = manager.play_game(engine1_white=False)
                
                with self.lock:
                    # Update results
                    for result in [result1, result2]:
                        if result == 1.0:
                            self.wins += 1
                        elif result == 0.0:
                            self.losses += 1
                        else:
                            self.draws += 1
                            
                    self.games_completed += 2
                    
                    # Print progress
                    if self.games_completed % 10 == 0:
                        self.print_progress()
                        
            except Exception as e:
                print(f"\nError in game pair {i}: {e}")
                with self.lock:
                    self.games_completed += 2
                    self.draws += 2  # Count errors as draws
                    
    def print_progress(self):
        """Print current progress"""
        total = self.wins + self.draws + self.losses
        if total == 0:
            return
            
        win_rate = self.wins / total
        draw_rate = self.draws / total
        loss_rate = self.losses / total
        
        # Simple ELO calculation
        score = (self.wins + 0.5 * self.draws) / total
        if 0 < score < 1:
            elo = 400 * math.log10(score / (1 - score))
        else:
            elo = 0
            
        elapsed = time.time() - self.start_time
        games_per_sec = total / elapsed if elapsed > 0 else 0
        
        print(f"\rGames: {total}/{self.num_games} | "
              f"W: {self.wins} ({win_rate:.1%}) "
              f"D: {self.draws} ({draw_rate:.1%}) "
              f"L: {self.losses} ({loss_rate:.1%}) | "
              f"ELO: {elo:+.1f} | "
              f"Speed: {games_per_sec:.1f} g/s", end='')
        sys.stdout.flush()
        
    def run(self):
        """Run the test"""
        print(f"Testing: {os.path.basename(self.test_engine)} vs {os.path.basename(self.base_engine)}")
        print(f"Games: {self.num_games} | Time: {self.time_ms}ms + {self.inc_ms}ms/move")
        print("-" * 80)
        
        # Validate engines
        if not os.path.exists(self.base_engine):
            print(f"Error: Base engine not found: {self.base_engine}")
            return
            
        if not os.path.exists(self.test_engine):
            print(f"Error: Test engine not found: {self.test_engine}")
            return
            
        # Make engines executable
        try:
            os.chmod(self.base_engine, 0o755)
            os.chmod(self.test_engine, 0o755)
        except:
            pass
            
        self.start_time = time.time()
        
        # Distribute games among threads
        games_per_thread = self.num_games // (2 * self.concurrency)  # Divide by 2 for pairs
        remaining = (self.num_games // 2) - (games_per_thread * self.concurrency)
        
        threads = []
        
        for i in range(self.concurrency):
            start_idx = i * games_per_thread
            end_idx = start_idx + games_per_thread
            if i == self.concurrency - 1:
                end_idx += remaining
                
            indices = list(range(start_idx, end_idx))
            thread = threading.Thread(target=self.run_games, args=(indices,))
            thread.start()
            threads.append(thread)
            
        # Wait for completion
        for thread in threads:
            thread.join()
            
        # Final results
        self.print_final_results()
        
    def print_final_results(self):
        """Print final results"""
        print("\n" + "=" * 80)
        
        total = self.wins + self.draws + self.losses
        if total == 0:
            print("No games completed!")
            return
            
        win_rate = self.wins / total
        draw_rate = self.draws / total
        loss_rate = self.losses / total
        
        score = (self.wins + 0.5 * self.draws) / total
        
        # ELO calculation
        if 0 < score < 1:
            elo = 400 * math.log10(score / (1 - score))
            
            # Confidence interval
            variance = score * (1 - score) / total
            std_dev = math.sqrt(variance)
            elo_std = 400 * std_dev / (score * (1 - score) * math.log(10))
            ci_lower = elo - 1.96 * elo_std
            ci_upper = elo + 1.96 * elo_std
        else:
            elo = ci_lower = ci_upper = 0
            
        # LOS (Likelihood of Superiority)
        if self.wins + self.losses > 0:
            los = 0.5 + 0.5 * math.erf((self.wins - self.losses) / 
                                       math.sqrt(2 * (self.wins + self.losses)))
        else:
            los = 0.5
            
        elapsed = time.time() - self.start_time
        
        print(f"Results: +{self.wins} ={self.draws} -{self.losses}")
        print(f"Win rate: {win_rate:.1%} | Draw rate: {draw_rate:.1%} | Loss rate: {loss_rate:.1%}")
        print(f"ELO difference: {elo:+.2f} [{ci_lower:+.2f}, {ci_upper:+.2f}] (95% CI)")
        print(f"LOS: {los:.1%}")
        print(f"Time: {elapsed:.1f}s ({total/elapsed:.1f} games/s)")
        
        # Save results
        results = {
            'base_engine': self.base_engine,
            'test_engine': self.test_engine,
            'games': total,
            'wins': self.wins,
            'draws': self.draws,
            'losses': self.losses,
            'elo': elo,
            'elo_ci': [ci_lower, ci_upper],
            'los': los,
            'time_seconds': elapsed,
            'time_control': f"{self.time_ms}+{self.inc_ms}",
            'timestamp': datetime.now().isoformat()
        }
        
        filename = f"test_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
            
        print(f"\nResults saved to: {filename}")
        
def main():
    if len(sys.argv) < 3:
        print("Usage: python simple_test.py <base_engine> <test_engine> [options]")
        print("\nOptions:")
        print("  --games N      Number of games to play (default: 100)")
        print("  --concurrency N Number of concurrent games (default: 1)")
        print("  --time MS      Initial time per game in milliseconds (default: 10000)")
        print("  --inc MS       Increment per move in milliseconds (default: 100)")
        sys.exit(1)
        
    base_engine = sys.argv[1]
    test_engine = sys.argv[2]
    
    # Parse options
    kwargs = {
        'num_games': 100,
        'concurrency': 1,
        'time_ms': 10000,
        'inc_ms': 100
    }
    
    i = 3
    while i < len(sys.argv):
        if sys.argv[i] == '--games' and i + 1 < len(sys.argv):
            kwargs['num_games'] = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == '--concurrency' and i + 1 < len(sys.argv):
            kwargs['concurrency'] = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == '--time' and i + 1 < len(sys.argv):
            kwargs['time_ms'] = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == '--inc' and i + 1 < len(sys.argv):
            kwargs['inc_ms'] = int(sys.argv[i + 1])
            i += 2
        else:
            i += 1
            
    tester = SimpleTester(base_engine, test_engine, **kwargs)
    tester.run()

if __name__ == '__main__':
    main()
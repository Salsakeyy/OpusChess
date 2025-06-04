#!/usr/bin/env python3
"""
Simple Chess Engine Tester - No external dependencies
Directly interfaces with UCI engines to run tests
Enhanced with depth and time per move tracking
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
        self.total_nodes = 0
        self.total_time_ms = 0
        self.search_count = 0
        # Add depth tracking
        self.total_depth = 0
        self.max_depth = 0
        self.depth_count = 0
        
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
        max_nodes = 0
        search_time = 0
        max_search_depth = 0
        
        while self.process and self.process.poll() is None:
            try:
                line = self.process.stdout.readline().strip()
                if line:
                    lines.append(line)
                    
                    # Parse UCI info lines for nodes, time, and depth
                    if line.startswith('info'):
                        parts = line.split()
                        for i, part in enumerate(parts):
                            if part == 'nodes' and i + 1 < len(parts):
                                try:
                                    nodes = int(parts[i + 1])
                                    max_nodes = max(max_nodes, nodes)
                                except ValueError:
                                    pass
                            elif part == 'time' and i + 1 < len(parts):
                                try:
                                    search_time = int(parts[i + 1])
                                except ValueError:
                                    pass
                            elif part == 'depth' and i + 1 < len(parts):
                                try:
                                    depth = int(parts[i + 1])
                                    max_search_depth = max(max_search_depth, depth)
                                except ValueError:
                                    pass
                                    
                if expected in line:
                    break
            except:
                break
                
        # Update statistics if we got a search result
        if max_nodes > 0:
            self.total_nodes += max_nodes
            self.total_time_ms += search_time
            self.search_count += 1
            
        # Update depth statistics
        if max_search_depth > 0:
            self.total_depth += max_search_depth
            self.max_depth = max(self.max_depth, max_search_depth)
            self.depth_count += 1
            
        return lines
    
    def get_avg_nps(self):
        """Get average nodes per second"""
        if self.total_time_ms > 0 and self.search_count > 0:
            return int((self.total_nodes * 1000) / self.total_time_ms)
        return 0
    
    def get_avg_depth(self):
        """Get average search depth"""
        if self.depth_count > 0:
            return self.total_depth / self.depth_count
        return 0.0
        
    def get_avg_time_per_move(self):
        """Get average time per move in milliseconds"""
        if self.search_count > 0:
            return self.total_time_ms / self.search_count
        return 0.0
        
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
        """Play a single game between engines - returns (result, engine1_stats, engine2_stats)"""
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
                    stats1 = {
                        'nps': engine1.get_avg_nps(),
                        'depth': engine1.get_avg_depth(),
                        'time_per_move': engine1.get_avg_time_per_move()
                    }
                    stats2 = {
                        'nps': engine2.get_avg_nps(),
                        'depth': engine2.get_avg_depth(),
                        'time_per_move': engine2.get_avg_time_per_move()
                    }
                    if current_engine == white_engine:
                        return (0.0 if engine1_white else 1.0), stats1, stats2  # White loses
                    else:
                        return (1.0 if engine1_white else 0.0), stats1, stats2  # Black loses
                    
                moves.append(move)
                
                # Check for game end
                ended, result = self.check_game_end(moves)
                if ended:
                    stats1 = {
                        'nps': engine1.get_avg_nps(),
                        'depth': engine1.get_avg_depth(),
                        'time_per_move': engine1.get_avg_time_per_move()
                    }
                    stats2 = {
                        'nps': engine2.get_avg_nps(),
                        'depth': engine2.get_avg_depth(),
                        'time_per_move': engine2.get_avg_time_per_move()
                    }
                    if result == "draw":
                        return 0.5, stats1, stats2
                    # Other end conditions would be handled here
                    
                # Check for time forfeit
                if (current_engine == white_engine and white_time < 100) or \
                   (current_engine == black_engine and black_time < 100):
                    # Time forfeit
                    stats1 = {
                        'nps': engine1.get_avg_nps(),
                        'depth': engine1.get_avg_depth(),
                        'time_per_move': engine1.get_avg_time_per_move()
                    }
                    stats2 = {
                        'nps': engine2.get_avg_nps(),
                        'depth': engine2.get_avg_depth(),
                        'time_per_move': engine2.get_avg_time_per_move()
                    }
                    if current_engine == white_engine:
                        return (0.0 if engine1_white else 1.0), stats1, stats2  # White loses on time
                    else:
                        return (1.0 if engine1_white else 0.0), stats1, stats2  # Black loses on time
                
                # Switch engines
                current_engine = black_engine if current_engine == white_engine else white_engine
                
            # Game too long - adjudicate as draw
            stats1 = {
                'nps': engine1.get_avg_nps(),
                'depth': engine1.get_avg_depth(),
                'time_per_move': engine1.get_avg_time_per_move()
            }
            stats2 = {
                'nps': engine2.get_avg_nps(),
                'depth': engine2.get_avg_depth(),
                'time_per_move': engine2.get_avg_time_per_move()
            }
            return 0.5, stats1, stats2
            
        except Exception as e:
            print(f"Error in game: {e}")
            stats1 = {
                'nps': engine1.get_avg_nps(),
                'depth': engine1.get_avg_depth(),
                'time_per_move': engine1.get_avg_time_per_move()
            }
            stats2 = {
                'nps': engine2.get_avg_nps(),
                'depth': engine2.get_avg_depth(),
                'time_per_move': engine2.get_avg_time_per_move()
            }
            return 0.5, stats1, stats2  # Draw on error
            
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
        
        # Enhanced statistics tracking
        self.test_engine_total_nps = 0
        self.base_engine_total_nps = 0
        self.test_engine_total_depth = 0.0
        self.base_engine_total_depth = 0.0
        self.test_engine_total_time = 0.0
        self.base_engine_total_time = 0.0
        self.stats_samples = 0
        
        # Progress
        self.games_completed = 0
        self.start_time = None
        
    def run_games(self, game_indices):
        """Worker function to run games"""
        manager = GameManager(self.test_engine, self.base_engine, self.time_ms, self.inc_ms)
        
        for i in game_indices:
            try:
                # Play pair of games with reversed colors
                result1, test_stats1, base_stats1 = manager.play_game(engine1_white=True)
                result2, test_stats2, base_stats2 = manager.play_game(engine1_white=False)
                
                with self.lock:
                    # Update results
                    for result in [result1, result2]:
                        if result == 1.0:
                            self.wins += 1
                        elif result == 0.0:
                            self.losses += 1
                        else:
                            self.draws += 1
                    
                    # Update statistics
                    for test_stats, base_stats in [(test_stats1, base_stats1), (test_stats2, base_stats2)]:
                        if test_stats['nps'] > 0:
                            self.test_engine_total_nps += test_stats['nps']
                            self.test_engine_total_depth += test_stats['depth']
                            self.test_engine_total_time += test_stats['time_per_move']
                            self.stats_samples += 1
                        if base_stats['nps'] > 0:
                            self.base_engine_total_nps += base_stats['nps']
                            self.base_engine_total_depth += base_stats['depth']
                            self.base_engine_total_time += base_stats['time_per_move']
                            
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
        
        # Calculate averages
        if self.stats_samples > 0:
            test_avg_nps = self.test_engine_total_nps // self.stats_samples
            base_avg_nps = self.base_engine_total_nps // self.stats_samples
            test_avg_depth = self.test_engine_total_depth / self.stats_samples
            base_avg_depth = self.base_engine_total_depth / self.stats_samples
            test_avg_time = self.test_engine_total_time / self.stats_samples
            base_avg_time = self.base_engine_total_time / self.stats_samples
        else:
            test_avg_nps = base_avg_nps = 0
            test_avg_depth = base_avg_depth = 0.0
            test_avg_time = base_avg_time = 0.0
        
        print(f"\rGames: {total}/{self.num_games} | "
              f"W: {self.wins} ({win_rate:.1%}) "
              f"D: {self.draws} ({draw_rate:.1%}) "
              f"L: {self.losses} ({loss_rate:.1%}) | "
              f"ELO: {elo:+.1f} | "
              f"Test NPS: {test_avg_nps:,} Depth: {test_avg_depth:.1f} Time: {test_avg_time:.0f}ms | "
              f"Base NPS: {base_avg_nps:,} Depth: {base_avg_depth:.1f} Time: {base_avg_time:.0f}ms | "
              f"Speed: {games_per_sec:.1f} g/s", end='')
        sys.stdout.flush()
        
    def run(self):
        """Run the test"""
        print(f"Testing: {os.path.basename(self.test_engine)} vs {os.path.basename(self.base_engine)}")
        print(f"Games: {self.num_games} | Time: {self.time_ms}ms + {self.inc_ms}ms/move")
        print("-" * 120)
        
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
        print("\n" + "=" * 120)
        
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
        
        # Calculate final averages
        if self.stats_samples > 0:
            test_avg_nps = self.test_engine_total_nps // self.stats_samples
            base_avg_nps = self.base_engine_total_nps // self.stats_samples
            test_avg_depth = self.test_engine_total_depth / self.stats_samples
            base_avg_depth = self.base_engine_total_depth / self.stats_samples
            test_avg_time = self.test_engine_total_time / self.stats_samples
            base_avg_time = self.base_engine_total_time / self.stats_samples
        else:
            test_avg_nps = base_avg_nps = 0
            test_avg_depth = base_avg_depth = 0.0
            test_avg_time = base_avg_time = 0.0
        
        print(f"Results: +{self.wins} ={self.draws} -{self.losses}")
        print(f"Win rate: {win_rate:.1%} | Draw rate: {draw_rate:.1%} | Loss rate: {loss_rate:.1%}")
        print(f"ELO difference: {elo:+.2f} [{ci_lower:+.2f}, {ci_upper:+.2f}] (95% CI)")
        print(f"LOS: {los:.1%}")
        print()
        print("Engine Statistics:")
        print(f"Test Engine  - NPS: {test_avg_nps:,} | Avg Depth: {test_avg_depth:.1f} | Avg Time/Move: {test_avg_time:.0f}ms")
        print(f"Base Engine  - NPS: {base_avg_nps:,} | Avg Depth: {base_avg_depth:.1f} | Avg Time/Move: {base_avg_time:.0f}ms")
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
            'test_engine_stats': {
                'avg_nps': test_avg_nps,
                'avg_depth': test_avg_depth,
                'avg_time_per_move': test_avg_time
            },
            'base_engine_stats': {
                'avg_nps': base_avg_nps,
                'avg_depth': base_avg_depth,
                'avg_time_per_move': base_avg_time
            },
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
        'time_ms': 100000,
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
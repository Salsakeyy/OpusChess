#!/usr/bin/env python3
"""
Chess Engine Testing Framework
Similar to fishtest for testing chess engine improvements
"""

import subprocess
import threading
import time
import math
import json
import os
import sys
import argparse
from datetime import datetime
from collections import defaultdict
import queue
import signal

class ChessMatch:
    """Manages a single game between two engines"""
    
    def __init__(self, engine1_path, engine2_path, time_control, opening_book=None):
        self.engine1_path = engine1_path
        self.engine2_path = engine2_path
        self.time_control = time_control
        self.opening_book = opening_book
        
    def play_game(self, white_engine, black_engine):
        """Play a single game and return result (1, 0.5, 0)"""
        # This is a simplified version - you'd need to implement proper game management
        # For now, we'll simulate with cutechess-cli or similar
        
        # Example command for cutechess-cli:
        cmd = [
            'cutechess-cli',
            '-engine', f'cmd={white_engine}', 'proto=uci',
            '-engine', f'cmd={black_engine}', 'proto=uci',
            '-each', f'tc={self.time_control}',
            '-rounds', '1',
            '-pgnout', '/dev/null',
            '-ratinginterval', '0'
        ]
        
        if self.opening_book:
            cmd.extend(['-openings', 'file=' + self.opening_book])
            
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            # Parse result from cutechess output
            # This is simplified - you'd need proper parsing
            if "1-0" in result.stdout:
                return 1.0
            elif "0-1" in result.stdout:
                return 0.0
            else:
                return 0.5
        except:
            # On error, count as draw
            return 0.5

class SPRT:
    """Sequential Probability Ratio Test implementation"""
    
    def __init__(self, alpha=0.05, beta=0.05, elo0=0, elo1=5):
        self.alpha = alpha
        self.beta = beta
        self.elo0 = elo0
        self.elo1 = elo1
        
        # Convert ELO to winning probability
        self.p0 = 1 / (1 + 10**(-elo0/400))
        self.p1 = 1 / (1 + 10**(-elo1/400))
        
        # Calculate bounds
        self.lower_bound = math.log(beta / (1 - alpha))
        self.upper_bound = math.log((1 - beta) / alpha)
        
        # Log likelihood ratio
        self.llr = 0.0
        
    def update(self, wins, draws, losses):
        """Update LLR based on game results"""
        n = wins + draws + losses
        if n == 0:
            return
            
        w = wins / n
        d = draws / n
        l = losses / n
        
        # Pentanomial model for chess results
        # This is a simplified version
        score = (wins + 0.5 * draws) / n
        
        if score > 0 and score < 1:
            # Log likelihood ratio calculation
            self.llr = n * (score * math.log(score/self.p0) + (1-score) * math.log((1-score)/(1-self.p0)))
            self.llr -= n * (score * math.log(score/self.p1) + (1-score) * math.log((1-score)/(1-self.p1)))
            
    def status(self):
        """Return test status: 'continue', 'accept', or 'reject'"""
        if self.llr <= self.lower_bound:
            return 'reject'
        elif self.llr >= self.upper_bound:
            return 'accept'
        else:
            return 'continue'

class EloCalculator:
    """Calculate ELO difference from game results"""
    
    @staticmethod
    def calculate(wins, draws, losses):
        """Calculate ELO difference with confidence interval"""
        n = wins + draws + losses
        if n == 0:
            return 0, 0, 0
            
        score = (wins + 0.5 * draws) / n
        
        # Prevent division by zero
        if score == 0:
            score = 0.001
        elif score == 1:
            score = 0.999
            
        # ELO difference
        elo = 400 * math.log10(score / (1 - score))
        
        # Confidence interval (simplified)
        variance = score * (1 - score) / n
        std_dev = math.sqrt(variance)
        elo_std = 400 * std_dev / (score * (1 - score) * math.log(10))
        
        # 95% confidence interval
        ci_lower = elo - 1.96 * elo_std
        ci_upper = elo + 1.96 * elo_std
        
        return elo, ci_lower, ci_upper

class EngineTest:
    """Main testing framework"""
    
    def __init__(self, base_engine, test_engine, **kwargs):
        self.base_engine = os.path.abspath(base_engine)
        self.test_engine = os.path.abspath(test_engine)
        
        # Test parameters
        self.time_control = kwargs.get('time_control', '10+0.1')
        self.num_games = kwargs.get('num_games', 10000)
        self.concurrency = kwargs.get('concurrency', 1)
        self.opening_book = kwargs.get('opening_book', None)
        
        # SPRT parameters
        self.sprt_alpha = kwargs.get('sprt_alpha', 0.05)
        self.sprt_beta = kwargs.get('sprt_beta', 0.05)
        self.sprt_elo0 = kwargs.get('sprt_elo0', -1.75)
        self.sprt_elo1 = kwargs.get('sprt_elo1', 0.25)
        
        # Results
        self.results = {
            'wins': 0,
            'draws': 0,
            'losses': 0,
            'crashes': 0
        }
        
        # Pentanomial statistics
        self.pentanomial = defaultdict(int)
        
        # Threading
        self.lock = threading.Lock()
        self.stop_flag = threading.Event()
        self.game_queue = queue.Queue()
        
        # Statistics
        self.start_time = None
        self.sprt = SPRT(self.sprt_alpha, self.sprt_beta, self.sprt_elo0, self.sprt_elo1)
        
    def worker(self):
        """Worker thread for running games"""
        match = ChessMatch(self.base_engine, self.test_engine, self.time_control, self.opening_book)
        
        while not self.stop_flag.is_set():
            try:
                game_num = self.game_queue.get(timeout=1)
            except queue.Empty:
                continue
                
            if game_num is None:
                break
                
            # Play two games with colors reversed
            results = []
            
            # Game 1: test=white, base=black
            result1 = match.play_game(self.test_engine, self.base_engine)
            results.append(result1)
            
            # Game 2: base=white, test=black  
            result2 = match.play_game(self.base_engine, self.test_engine)
            results.append(1 - result2)  # Invert for test engine perspective
            
            # Update results
            with self.lock:
                for result in results:
                    if result == 1:
                        self.results['wins'] += 1
                    elif result == 0:
                        self.results['losses'] += 1
                    else:
                        self.results['draws'] += 1
                        
                # Update pentanomial (simplified)
                pair_result = sum(results)
                if pair_result == 2:
                    self.pentanomial['WW'] += 1
                elif pair_result == 1.5:
                    self.pentanomial['WD'] += 1
                elif pair_result == 1:
                    self.pentanomial['WL/DD'] += 1
                elif pair_result == 0.5:
                    self.pentanomial['LD'] += 1
                else:
                    self.pentanomial['LL'] += 1
                    
            self.game_queue.task_done()
            
    def run(self):
        """Run the test"""
        self.start_time = time.time()
        
        # Validate engines exist
        if not os.path.exists(self.base_engine):
            print(f"Error: Base engine not found: {self.base_engine}")
            return False
            
        if not os.path.exists(self.test_engine):
            print(f"Error: Test engine not found: {self.test_engine}")
            return False
            
        print(f"Testing {self.test_engine} vs {self.base_engine}")
        print(f"Time control: {self.time_control}")
        print(f"SPRT: elo0={self.sprt_elo0} elo1={self.sprt_elo1} alpha={self.sprt_alpha} beta={self.sprt_beta}")
        print(f"Concurrency: {self.concurrency}")
        print("-" * 80)
        
        # Start worker threads
        workers = []
        for _ in range(self.concurrency):
            t = threading.Thread(target=self.worker)
            t.start()
            workers.append(t)
            
        # Queue games
        for i in range(self.num_games // 2):  # Divide by 2 because we play pairs
            self.game_queue.put(i)
            
        # Monitor progress
        try:
            while not self.game_queue.empty() or any(t.is_alive() for t in workers):
                self.print_status()
                time.sleep(1)
                
                # Check SPRT
                with self.lock:
                    self.sprt.update(self.results['wins'], self.results['draws'], self.results['losses'])
                    status = self.sprt.status()
                    
                if status != 'continue':
                    print(f"\nSPRT {status}ed!")
                    self.stop_flag.set()
                    break
                    
        except KeyboardInterrupt:
            print("\nStopping test...")
            self.stop_flag.set()
            
        # Clean up
        for _ in workers:
            self.game_queue.put(None)
            
        for t in workers:
            t.join()
            
        # Final results
        print("\n" + "=" * 80)
        self.print_final_results()
        
        return True
        
    def print_status(self):
        """Print current test status"""
        with self.lock:
            total = self.results['wins'] + self.results['draws'] + self.results['losses']
            if total == 0:
                return
                
            elo, ci_lower, ci_upper = EloCalculator.calculate(
                self.results['wins'], self.results['draws'], self.results['losses']
            )
            
            los = self.calculate_los()
            
            elapsed = time.time() - self.start_time
            games_per_sec = total / elapsed if elapsed > 0 else 0
            
            # Clear line and print status
            print(f"\rGames: {total:>6} | "
                  f"W: {self.results['wins']:>5} D: {self.results['draws']:>5} L: {self.results['losses']:>5} | "
                  f"ELO: {elo:>+6.2f} [{ci_lower:>+6.2f},{ci_upper:>+6.2f}] | "
                  f"LOS: {los:>6.1%} | "
                  f"LLR: {self.sprt.llr:>6.2f} [{self.sprt.lower_bound:>+5.2f},{self.sprt.upper_bound:>+5.2f}] | "
                  f"Speed: {games_per_sec:>5.1f} g/s", end='')
            
    def calculate_los(self):
        """Calculate Likelihood of Superiority"""
        wins = self.results['wins'] + 0.5 * self.results['draws']
        losses = self.results['losses'] + 0.5 * self.results['draws']
        
        if wins + losses == 0:
            return 0.5
            
        # Using normal approximation
        n = wins + losses
        p = wins / n
        std = math.sqrt(p * (1 - p) / n)
        
        # Z-score for p > 0.5
        z = (p - 0.5) / std if std > 0 else 0
        
        # Convert to probability using normal CDF approximation
        los = 0.5 * (1 + math.erf(z / math.sqrt(2)))
        
        return los
        
    def print_final_results(self):
        """Print final test results"""
        total = self.results['wins'] + self.results['draws'] + self.results['losses']
        if total == 0:
            print("No games completed")
            return
            
        elo, ci_lower, ci_upper = EloCalculator.calculate(
            self.results['wins'], self.results['draws'], self.results['losses']
        )
        
        los = self.calculate_los()
        elapsed = time.time() - self.start_time
        
        print(f"Test: {os.path.basename(self.test_engine)} vs {os.path.basename(self.base_engine)}")
        print(f"Games: {total}")
        print(f"Result: W:{self.results['wins']} D:{self.results['draws']} L:{self.results['losses']}")
        print(f"ELO: {elo:+.2f} [{ci_lower:+.2f},{ci_upper:+.2f}] (95% CI)")
        print(f"LOS: {los:.1%}")
        print(f"LLR: {self.sprt.llr:.2f} [{self.sprt.lower_bound:.2f},{self.sprt.upper_bound:.2f}]")
        print(f"Time: {elapsed:.1f}s ({total/elapsed:.1f} games/s)")
        
        # Pentanomial
        print(f"\nPentanomial: {dict(self.pentanomial)}")
        
        # Save results
        self.save_results()
        
    def save_results(self):
        """Save results to JSON file"""
        results = {
            'base_engine': self.base_engine,
            'test_engine': self.test_engine,
            'time_control': self.time_control,
            'games': self.results,
            'pentanomial': dict(self.pentanomial),
            'sprt': {
                'elo0': self.sprt_elo0,
                'elo1': self.sprt_elo1,
                'alpha': self.sprt_alpha,
                'beta': self.sprt_beta,
                'llr': self.sprt.llr,
                'status': self.sprt.status()
            },
            'timestamp': datetime.now().isoformat()
        }
        
        filename = f"test_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
            
        print(f"\nResults saved to: {filename}")

def main():
    parser = argparse.ArgumentParser(description='Chess Engine Testing Framework')
    parser.add_argument('base', help='Path to base engine')
    parser.add_argument('test', help='Path to test engine')
    parser.add_argument('--games', type=int, default=10000, help='Maximum number of games')
    parser.add_argument('--concurrency', type=int, default=1, help='Number of concurrent games')
    parser.add_argument('--tc', default='10+0.1', help='Time control (e.g., 10+0.1)')
    parser.add_argument('--book', help='Opening book file')
    parser.add_argument('--sprt-elo0', type=float, default=-1.75, help='SPRT H0 ELO')
    parser.add_argument('--sprt-elo1', type=float, default=0.25, help='SPRT H1 ELO')
    parser.add_argument('--sprt-alpha', type=float, default=0.05, help='SPRT alpha')
    parser.add_argument('--sprt-beta', type=float, default=0.05, help='SPRT beta')
    
    args = parser.parse_args()
    
    # Handle Ctrl+C gracefully
    signal.signal(signal.SIGINT, signal.default_int_handler)
    
    test = EngineTest(
        args.base,
        args.test,
        num_games=args.games,
        concurrency=args.concurrency,
        time_control=args.tc,
        opening_book=args.book,
        sprt_elo0=args.sprt_elo0,
        sprt_elo1=args.sprt_elo1,
        sprt_alpha=args.sprt_alpha,
        sprt_beta=args.sprt_beta
    )
    
    test.run()

if __name__ == '__main__':
    main()
# Othello Parallelize

## Files

* `main.cpp` : game controller

## Implementation-MCTS
Loop in limited time
* Tree Traversal
  * $UCT = \frac{w_i}{n_i} + C*\sqrt{\ln{N}/n_i}$
* Expansion
* Rollout
* Backpropagation

### Details

* Add an delta value to divisor in case divisor is zero
* Count amount of disks on each move

### Optimization (iterations / milliseconds, from initial board)
* Original MCTS: 8306 / 2000
* State Compress: 10146 / 2000
* Comparison Optimize((0 <= x < 8) -> (!(x & (~7)))): 10350 / 2000
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
* 
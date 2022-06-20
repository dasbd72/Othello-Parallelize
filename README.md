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
### Root Parallelization

## Compile Commands
### `player.cpp`

g++ --std=c++17 -O3 -Wextra -Wall -fsanitize=address -g player.cpp -o player_dbg; ./player_dbg testcase/state0 action
g++ --std=c++17 -O3 player.cpp -o player
make; ./player testcase/state0 action
make; ./main ./player ./players/player_109062131
make; ./main ./players/player_109062131 ./player
make; ./main ./player ./players/player_random
make; ./main ./player ./players/baseline1
make; ./main ./player ./players/baseline2
make; ./main ./player ./players/baseline3
make; ./main ./player ./players/baseline4
make; ./main ./player ./players/baseline5

### `player_parallel.cpp`

g++ --std=c++17 -O3 -fopenmp -pthread -Wextra -Wall -fsanitize=address -g player_parallel.cpp -o player_dbg; srun -c12 ./player_dbg testcase/state0 action
g++ --std=c++17 -O3 -fopenmp -pthread player_parallel.cpp -o player_parallel
make; srun -c12 ./player_parallel testcase/state0 action
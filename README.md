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
* Independent Simulation in Rollout(using node): 17600 / 2000
* Independent Rollout(not using node): 20084 / 2000
  * Reduce time copying state
* Same code using srun: 26568 / 2000
* Pruning on directions: 
* rand() is locking, use rand_r(&seed) instead

### Direct Parallelization
* No speedup
### Leaf Parallelization
* N(games) = iterations * numthreads
* Multiple Rollout at once
### Root Parallelization
* run 

## Compile Commands
### `player.cpp`

g++ --std=c++17 -O3 -Wextra -Wall -fsanitize=address -g player.cpp -o player_dbg; ./player_dbg testcase/state0 action
g++ --std=c++17 -O3 -Wextra -Wall player_test.cpp -o player_test; srun ./player_test testcase/state0 action 1000 1
make; srun ./player testcase/state0 action 2000 1
make; srun -c4 ./player testcase/state0 action
make; srun -c8 ./player testcase/state0 action

### `player_leaf_parallel.cpp`

g++ --std=c++17 -O3 -fopenmp -pthread -Wextra -Wall -fsanitize=address -g player_leaf_parallel.cpp -o player_dbg; srun -c8 ./player_dbg testcase/state0 action
make; srun -c8 ./player_leaf_parallel testcase/state0 action 2000 1

### `player_root_parallel.cpp`

g++ --std=c++17 -O3 -fopenmp -pthread -Wextra -Wall -fsanitize=address -g player_root_parallel.cpp -o player_dbg; srun -c8 ./player_dbg testcase/state0 action
make; srun -c8 ./player_root_parallel testcase/state0 action 2000 1

### `player_tree_parallel.cpp`

g++ --std=c++17 -O3 -fopenmp -pthread -Wextra -Wall -fsanitize=address -g player_tree_parallel.cpp -o player_dbg; srun -c8 ./player_dbg testcase/state0 action
g++ --std=c++17 -O3 -fopenmp -pthread -Wextra -Wall player_tree_parallel.cpp -o player_dbg; srun -c8 ./player_dbg testcase/state0 action 2000 1
make; srun ./player_tree_parallel testcase/state0 action 2000 1
make; srun -c4 ./player_tree_parallel testcase/state0 action 2000 1
make; srun -c8 ./player_tree_parallel testcase/state0 action 2000 1

# Testbench

make; ./testbench 0 1 8 1000
make; ./testbench 0 2 8 1000
make; ./testbench 0 3 8 1000
make; ./testbench 1 2 8 1000
make; ./testbench 1 3 8 1000
make; ./testbench 2 3 8 1000

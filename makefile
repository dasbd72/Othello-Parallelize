SOURCES		= $(wildcard *.cpp)
EXE			= $(SOURCES:%.cpp=%)
SEQ			= main player testbench
CPU			= player_parallel player_tree_parallel player_root_parallel player_leaf_parallel
# GPU			= player_leaf_parallel_cuda
OTHER		= action state gamelog.txt

.PHONY: all clean

all: $(SEQ) $(CPU) $(GPU)

$(SEQ): % : %.cpp
	g++ -Wall -Wextra --std=c++17 -O3 -o $@ $<

$(CPU): % : %.cpp
	g++ -Wall -Wextra -pthread -fopenmp --std=c++17 -O3 -o $@ $<

$(GPU): % : %.cu
	nvcc --std=c++17 -O3 -Xptxas=-v -arch=sm_61 -o $@ $<

clean:
	rm -f $(EXE) $(OTHER)

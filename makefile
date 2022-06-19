SOURCES		= $(wildcard *.cpp)
EXE			= $(SOURCES:%.cpp=%)
SEQ			= main player
CPU			= player_root_parallel player_leaf_parallel
OTHER		= action state gamelog.txt

.PHONY: all clean

all: $(SEQ) $(CPU)

$(SEQ): % : %.cpp
	g++ -Wall -Wextra --std=c++17 -O3 -o $@ $<

$(CPU): % : %.cpp
	g++ -Wall -Wextra  -pthread --std=c++17 -O3 -o $@ $<

clean:
	rm -f $(EXE) $(OTHER)

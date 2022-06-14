CXX			= g++
CXXFLAGS	= --std=c++17 -O3
SOURCES		= $(wildcard *.cpp)
EXE			= $(SOURCES:%.cpp=%)
OTHER		= action state gamelog.txt

.PHONY: all clean

all: $(EXE)

$(EXE): % : %.cpp
	$(CXX) -Wall -Wextra $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(EXE) $(OTHER)

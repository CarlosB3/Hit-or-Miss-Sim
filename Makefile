CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wshadow -Wconversion -pedantic

SIM_SRC := src/main.cpp src/trace.cpp src/cache/cache.cpp
GEN_SRC := tools/gen_traces.cpp

all: sim gen

sim: $(SIM_SRC)
	$(CXX) $(CXXFLAGS) -Isrc -Isrc/cache $(SIM_SRC) -o $@

gen: $(GEN_SRC)
	$(CXX) $(CXXFLAGS) $(GEN_SRC) -o $@

clean:
	rm -f sim gen

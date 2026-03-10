CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wshadow -Wconversion -pedantic
DBGFLAGS := -g -O0
SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

SRC := $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
TOOL := tools/gen_traces.cpp

all: sim gen

sim: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

gen: $(TOOL)
	$(CXX) $(CXXFLAGS) $(TOOL) -o $@

debug: CXXFLAGS := -std=c++17 $(DBGFLAGS) -Wall -Wextra -Wshadow -Wconversion -pedantic
debug: sim gen

sanitize: CXXFLAGS := -std=c++17 $(DBGFLAGS) $(SANFLAGS) -Wall -Wextra -Wshadow -Wconversion -pedantic
sanitize: sim gen

clean:
	rm -f sim gen

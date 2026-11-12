# One rule per directory; every program is a single .cpp that includes headers from src/.
CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -march=native -fopenmp -Wall -Wno-unused-result
HDRS := $(wildcard src/*.hpp)
bin/%: src/%.cpp $(HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $<
bin/%: verify/%.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $<
bin/%: sim/%.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $<

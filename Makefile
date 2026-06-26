# tiny makefile. nothing fancy, just compiles the two sources into one binary.
# run `make` to build, `make test` to run the self checks, `make clean` to tidy up

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET    = option_pricing
SRC       = main.cpp black_scholes.cpp

$(TARGET): $(SRC) black_scholes.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# build then immediately run the built in sanity checks
test: $(TARGET)
	./$(TARGET) selftest

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: test clean

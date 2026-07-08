CXX      = g++
CXXFLAGS = -std=c++17 -O2 -w -fopenmp -Isrc
SRC      = $(wildcard src/*.cpp)
kbtoytracer: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)
test: kbtoytracer
	python3 tests/run_tests.py
clean:
	rm -f kbtoytracer *.ppm *.png

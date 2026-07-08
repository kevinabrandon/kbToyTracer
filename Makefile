CXX      = g++
CXXFLAGS = -std=c++17 -O2 -w -fopenmp -Isrc
SRC      = $(wildcard src/*.cpp)
HDR      = $(wildcard src/*.h)
kbtoytracer: $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)
test: kbtoytracer
	python3 tests/run_tests.py
clean:
	rm -f kbtoytracer *.ppm *.png

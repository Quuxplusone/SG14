CXXFLAGS=-std=c++14 -O3 -W

bench: *.cc *.h
	$(CXX) $(CXXFLAGS) flatset-benchmark.cc -I../SG14 -I/usr/local/include -L/usr/local/lib -lbenchmark -o $@


CXX = clang++
CXXFLAGS = -std=c++11 -stdlib=libc++ -Werror -pedantic-errors
OUTDIR = ./build
TESTS_DEPS = tests/main.cpp

all: clean test

clean:
	rm -rf $(OUTDIR)/*

test: $(TESTS_DEPS)
	mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) ./tests/main.cpp -o $(OUTDIR)/test.a
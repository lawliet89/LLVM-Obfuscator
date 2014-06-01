OBF_BUILD = ./obf.sh
BUILD_DIR = build

CPP = $(BUILD_DIR)/Release+Asserts/bin/clang++
CPP_FLAGS = -O3 -Wall -std=c++11

OBF_FLAGS ?=

all: test/get_input.o test/get_input_obf.o test/generator test/stack-sort\
	test/stack-sort-obf test/hanoi test/hanoi-obf\
	test/mergesort test/mergesort-obf\
	test/radixsort test/radixsort-obf\
	test/quicksort test/quicksort-obf\
	test/bubblesort test/bubblesort-obf

clean-obf:
	rm test/*-obf

test/get_input.o: get_input.cpp get_input.h
	$(CPP) $(CPP_FLAGS) -c -o test/get_input.o get_input.cpp

test/get_input_obf.o: get_input.cpp get_input.h
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS)\
		-c -o test/get_input_obf.o get_input.cpp

test/stack-sort: stack-sort.cpp test/get_input.o
	$(CPP) $(CPP_FLAGS) -o test/stack-sort stack-sort.cpp test/get_input.o

test/stack-sort-obf: stack-sort.cpp test/get_input_obf.o
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS) \
		-o test/stack-sort-obf stack-sort.cpp test/get_input_obf.o

test/hanoi: hanoi.cpp
	$(CPP) $(CPP_FLAGS) -o test/hanoi hanoi.cpp

test/hanoi-obf: hanoi.cpp
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS) -o test/hanoi-obf hanoi.cpp

test/mergesort: mergesort.cpp test/get_input.o
	$(CPP) $(CPP_FLAGS) -o test/mergesort mergesort.cpp test/get_input.o

test/mergesort-obf: mergesort.cpp test/get_input_obf.o
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS) \
		-o test/mergesort-obf mergesort.cpp test/get_input_obf.o

test/radixsort: radixsort.cpp test/get_input.o
	$(CPP) $(CPP_FLAGS) -o test/radixsort radixsort.cpp test/get_input.o

test/radixsort-obf: radixsort.cpp test/get_input_obf.o
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS) \
		-o test/radixsort-obf radixsort.cpp test/get_input_obf.o

test/quicksort: quicksort.cpp test/get_input.o
	$(CPP) $(CPP_FLAGS) -o test/quicksort quicksort.cpp test/get_input.o

test/quicksort-obf: quicksort.cpp test/get_input_obf.o
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS) \
		-o test/quicksort-obf quicksort.cpp test/get_input_obf.o
test/bubblesort: bubblesort.cpp test/get_input.o
	$(CPP) $(CPP_FLAGS) -o test/bubblesort bubblesort.cpp test/get_input.o

test/bubblesort-obf: bubblesort.cpp test/get_input_obf.o
	$(OBF_BUILD) $(CPP_FLAGS) $(OBF_FLAGS) \
		-o test/bubblesort-obf bubblesort.cpp test/get_input_obf.o


test/generator: generator.cpp
	$(CPP) $(CPP_FLAGS) -o test/generator generator.cpp

clean:
	rm -f test/*

.PHONY: all clean clean-obf

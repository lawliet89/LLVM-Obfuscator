// Quicksort - see makefile for compiling options with G++.
// Will not compile in VS2012 due to initialiser syntax in main() for
// std::vector
#include <iostream>
#include <vector>
#include <algorithm> // Required
#include <iterator>  //Required
#include <stdlib.h>  // Required
#include <time.h>    // Required
#include "get_input.h"

// Function Prototypes
int randomNumber(int start, int end); // Generate a number between start and end

// Perform QuickSort
// Data provided will be modified
template <typename Iterator> void bubblesort(Iterator start, Iterator end) {
  int size = (end - start);
  if (size <= 0)
    return; // Base case

  bool swapped = false;
  do {
    swapped = false;
    for (Iterator it = start; it != end - 1; ++it) {
     if (*it > *(it + 1)){
	std::swap(*it, *(it+1));
	swapped = true;
     }
    }
  } while(swapped);

 
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need 1 input file name";
  }
  // Lazy to push manually to stack.
  std::vector<int> input = getInput(argv[1]);

  bubblesort(input.begin(), input.end());

  // C++11 ranged based for loops
  // http://www.cprogramming.com/c++11/c++11-ranged-for-loop.html
  for (int it : input) {
    std::cout << it << "\n";
  }

  return 0;
}

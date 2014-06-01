// Quicksort - see makefile for compiling options with G++.
// Will not compile in VS2012 due to initialiser syntax in main() for
// std::vector
#include <cstdio> // Using this to not use std::cout and prevent throws
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
        // We are going to use the following idiom to prevent std::swap throws	
        typename std::iterator_traits<Iterator>::value_type value = *it;
        *(it) = *(it + 1);
        *(it + 1) = value;
	swapped = true;
     }
    }
  } while(swapped);

 
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Needs input file");
    return 1;
  }
  // Lazy to push manually to stack.
  std::vector<int> input = getInput(argv[1]);

  bubblesort(input.begin(), input.end());

  // C++11 ranged based for loops
  // http://www.cprogramming.com/c++11/c++11-ranged-for-loop.html
  for (int it : input) {
    printf("%d\n", it); 
  }

  return 0;
}

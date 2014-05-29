// Quicksort - see makefile for compiling options with G++.
// Will not compile in VS2012 due to initialiser syntax in main() for
// std::vector
#include <iostream>
#include <vector>
#include <algorithm> // Required
#include <iterator>  //Required
#include <stdlib.h>  // Required
#include <time.h>    // Required
#include <fstream>

std::vector<int> getInput(const char *filename) {
  std::ifstream file(filename);

  std::vector<int> results;
  while (file.good()) {
    int number;
    file >> number;
    results.push_back(number);
  }

  return results;
}


// Function Prototypes
int randomNumber(int start, int end); // Generate a number between start and end

// Perform QuickSort
// Data provided will be modified
template <typename Iterator> void quickSort(Iterator start, Iterator end) {
  int size = (end - start);
  if (size <= 0)
    return; // Base case

  // Partition - in place partitioning that involves only swapping
  // https://class.coursera.org/algo/lecture/preview/22 - O(n) time with no
  // extra memory++
  /*
      Assume pivot is first element of array
      Otherwise swap pivot with first element

      Idea: Array A is in this form
          pivot | < p | > p | unpartitioned/unseen

      Let i = index of the first item > p
      Let j = index of the first item unpartitioned

      Let i = 1
      Let j = 1
      Let p = pivot value

      for j < size
          if A[i] < p
              swap A[j] with A[i]
              i++
      swap A[0] with A[i-1]
  */
  // Get pivot element
  int pivotIndex = randomNumber(0, size);
  typename std::iterator_traits<Iterator>::value_type pivot =
      *(start + pivotIndex);

  if (pivotIndex != 0)
    std::swap(*(start + pivotIndex), *start);

  int i = 1;
  for (int j = 1; j < size; j++) {
    if (*(start + j) < pivot) {
      std::swap(*(start + j), *(start + i));
      i++;
    }
  }

  std::swap(*start, *(start + i - 1));

  quickSort(start, start + i - 1);
  quickSort(start + i, end);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need 1 input file name";
  }
  // Lazy to push manually to stack.
  std::vector<int> input = getInput(argv[1]);

  quickSort(input.begin(), input.end());

  // C++11 ranged based for loops
  // http://www.cprogramming.com/c++11/c++11-ranged-for-loop.html
  for (int it : input) {
    std::cout << it << "\n";
  }

  return 0;
}

// Generate a number between start and end
int randomNumber(int start, int end) {
  // Seed the randomiser
  srand(time(NULL));

  return rand() % end + start;
}

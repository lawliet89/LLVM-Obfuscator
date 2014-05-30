// http://en.literateprograms.org/Merge_sort_(C_Plus_Plus)
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include "get_input.h"

// Note: InputIterator must allow the minus operator -- i.e. RandomAccess
// The end element is AFTER the last element
// output refers to the iterator for the first output item
// You can mix and match different input and output iterators (i.e. vectors,
// arrays etc.)
template <typename InputIterator, typename OutputIterator>
void mergesort(InputIterator start, InputIterator end, OutputIterator output) {
  int size = end - start;

  if (size == 1) {
    // Oh, base case
    *output = *start;
    return;
  }

  // Split
  int split = size / 2;

  // Sort Left
  mergesort(start, start + split, output);
  // Sort Right
  mergesort(start + split, end, output + split);

  // In place merging
  inplace_merge (output, output+split, output+size);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need 1 input file name";
  }
  // Lazy to push manually to stack.
  std::vector<int> input = getInput(argv[1]);
  std::vector<int> output(input.size());

  mergesort(input.begin(), input.end(), output.begin());

  for (auto number : output) {
    std::cout << number << "\n";
  }
  return 0;
}

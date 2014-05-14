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
  // Or... use
  // inplace_merge (output, output+split, output+size);
  OutputIterator left = output, right = output + split,
                 rightEnd = output + size;

  for (; left < right; left++) {
    if (*right < *left) {
      // If the right item is bigger than the left then we will move the right
      // item to the current position in the left
      // Get the type of the value
      typename std::iterator_traits<OutputIterator>::value_type
          value; // See
                 // http://www.cplusplus.com/reference/std/iterator/iterator_traits/
      std::swap(value, *left); // Use swap to prevent throwing. Now value has
                               // the value of left
      std::swap(*left, *right); // And left has the value of right

      // Now let's insert the former left value into the correct spot on the
      // right
      // Since the right is also sorted, we will move all the items on the right
      // one slot to the left until we get the correct position
      OutputIterator i = right + 1;
      while (i != rightEnd && *i < value) {
        std::swap(*i, *(i - 1));
        i++;
      }
      // When we are here, i-1 will be an empty slot to put value in
      std::swap(*(i - 1), value);
    }
  }
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
    std::cout << number << " ";
  }
  return 0;
}

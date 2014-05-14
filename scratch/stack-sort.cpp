#include "get_input.h"
#include <stack>
#include <iostream>

#include <vector> // for lazy man initialisation

using namespace std;

/*
    Using two stacks ONLY, sort a stack with biggest item on top
*/

template <typename T> stack<T> stackSort(stack<T> items);

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need 1 input file name";
  }
  // Lazy to push manually to stack.
  vector<int> input = getInput(argv[1]);

  stack<int> inputStack;

  for (int number : input) {
    inputStack.push(number);
  }

  stack<int> sorted = stackSort(inputStack);

  while (!sorted.empty()) {
    cout << sorted.top() << " ";
    sorted.pop();
  }
  cout << endl;
}

template <typename T> stack<T> stackSort(stack<T> items) {
  stack<T> sorted;

  while (!items.empty()) {
    T item = items.top();
    items.pop();

    int counter = 0;
    while (!sorted.empty() && sorted.top() < item) {
      items.push(sorted.top());
      sorted.pop();
      ++counter;
    }

    sorted.push(item);

    for (int i = 0; i < counter; ++i) {
      sorted.push(items.top());
      items.pop();
    }
  }

  return sorted;
}

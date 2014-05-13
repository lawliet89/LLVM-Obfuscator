#include "get_input.h"
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

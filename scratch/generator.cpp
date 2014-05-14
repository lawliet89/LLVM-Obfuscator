#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <climits>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: number [percentage sorted]\n";
    return 0;
  }

  unsigned count = atoi(argv[1]);
  double percent = 0.f;

  if (argc > 2) {
    double check = atof(argv[2]);
    if (check > 1.f || check < 0.f) {
      std::cerr << "Express percentage as an integer in the range [0, 1]\n";
      return 0;
    }
    percent = check;
  }

  std::minstd_rand engine;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  engine.seed(seed);

  std::vector<int> numbers(count);
  std::uniform_int_distribution<int> distribution(INT_MIN, INT_MAX);

  for (unsigned i = 0; i < count; ++i) {
    numbers[i] = distribution(engine);
  }

  if (percent > 0.f) {
    std::sort(numbers.begin(), numbers.end());

    if (percent < 1.f) {
      std::bernoulli_distribution trial(1.f -percent);
      std::uniform_int_distribution<int> randomIndex(0, count - 1);

      for (unsigned i = 0; i < count; ++i) {
        if (trial(engine)) {
          std::swap(numbers[i], numbers[randomIndex(engine)]);
        }
      }
    }
  }

  for (auto no : numbers) {
    std::cout << no << " ";
  }
}

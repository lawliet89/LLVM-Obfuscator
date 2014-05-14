#include <iostream>
#include <algorithm>
#include <stack>

using namespace std;

void moveDisks(int number, stack<int> &origin, stack<int> &destination,
               stack<int> &buffer);
void moveTop(stack<int> &origin, stack<int> &destination);

int main(int argc, char **argv) {
  int number;

  if (argc < 2) {
    cout << "Enter number of disks: ";
    cin >> number;
  } else {
    number = atoi(argv[1]);
  }

  stack<int> towers[3];

  for (int i = number; i > 0; --i) {
    towers[0].push(i);
  }

  moveDisks(number, towers[0], towers[1], towers[2]);

  for (int j = 0; j < 3; j++) {
    if (towers[j].empty())
      continue;
    for (int i = 0; i < number; ++i) {
      cout << towers[j].top() << "\n";
      towers[j].pop();
    }
    break;
  }
}

void moveDisks(int number, stack<int> &origin, stack<int> &destination,
               stack<int> &buffer) {
  if (number <= 0)
    return;

  moveDisks(number - 1, origin, buffer, destination);

  moveTop(origin, destination);

  moveDisks(number - 1, buffer, destination, origin);
}

void moveTop(stack<int> &origin, stack<int> &destination) {
  int disk = origin.top();
  destination.push(disk);
  // You might want to have a better way of saying "moving x form tower y to
  // tower z"
  // Probably using a wrapper class around stack for a tower
  // I was lazy.
  // cout << "Moving " << disk << "\n";
  origin.pop();
}

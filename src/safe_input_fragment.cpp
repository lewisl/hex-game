#include <iostream>
#include <limits>

int main() {
  int data;

  while (true) {
    std::cout << "Enter an integer: ";
    std::cin >> data;

    if (std::cin.fail()) {
      std::cout << "Invalid input, please try again.\n";
      std::cin.clear(); // clear the error flag
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard the input
    } else {
      break; // break out of the loop if input is valid
    }
  }

  std::cout << "You entered: " << data << '\n';

  return 0;
}

#include <iostream>
#include <vector>
#include <iterator>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
//  auto it = std::prev(vec.end()); // get an iterator to the last element
//  int& nextToLast = *(--it); // decrement the iterator and dereference to get the next-to-last element

    auto it = vec.end() - 2; // get an iterator to the last element
    int& nextToLast = *(it); // decrement the iterator and dereference to get the next-to-last element
    

    std::cout << "The next-to-last element is: " << nextToLast << std::endl;
    return 0;
}

#include <iostream>
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>

using namespace std;

// for random shuffling of board moves
random_device rd;
mt19937 randgen(rd());

struct Timing {
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    
    void reset() {
        start = std::chrono::steady_clock::now();
    }
    
    void stop() {
        end = std::chrono::steady_clock::now();
    }
    
    double ticks() {
        return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    }
};


int main() {
    const int n = 121;
    std::vector<int> vec(n);

    // initialize the vector with randomized hex game markers: 0->empty, 1, 2
    for (auto& elem : vec) {
        elem = rand() % 3;
    }

    // make a copy and fill empty positions randomly
    Timing this_timer;
    this_timer.reset();
    vector<int> sim(vec);  // copy
    
    for (auto& elem : sim) {
        if (elem == 0) {
            elem = rand() % 2 + 1;  // replace empty with random player markers
        }
    }
    
//  auto end = std::chrono::high_resolution_clock::now();
//  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    this_timer.stop(); 
    std::cout << "\nFilling the board with random markers took: " << this_timer.ticks() << " seconds" << std::endl;
    
    cout << "Result of initializing sim board positions:\n";
    int ctr = 0;
    for (const auto & elem : sim) {
        ctr++;
        cout << elem << " ";
        if (ctr % 10 == 0) cout << endl;
        
    }
    
    // an approach using indices to empty positions and shuffle
    
    // initialize the vector with randomized hex game markers: 0->empty, 1, 2
    for (auto& elem : vec) {
        elem = rand() % 3;
    }
    
    // find the indices to empties
    vector<int> empty_indices;
    vector<int> alternating(n);
    int alternate_marker=1;
    
    cout << endl;
    // initialize alternating
    for (auto & elem : alternating) {
        elem = alternate_marker;
        alternate_marker = alternate_marker == 1 ? 2 : 1;
    }
    
    cout << "\nThese are alternating markers to use with the shuffle approach: \n";
    for (const auto & elem : alternating) {
        ctr++;
        cout << elem << " ";
        if (ctr % 10 == 0) cout << endl;
    }
    
    
    this_timer.reset();
    
    ctr =0;
    empty_indices.clear();
    for (const int & elem : vec) {
        if (elem == 0) 
            empty_indices.push_back(ctr);
        ctr++;
    }
    
    this_timer.stop();
    cout << "\nFinding all of the empty indices and putting into a vector took " << this_timer.ticks() << " seconds\n";
    
    cout << "\nsize of empty_indices " << empty_indices.size() << endl;
    
    // verify if the empty indices are actually empty
    
    cout << "\nare these positions empty?\n";
    for (auto idx : empty_indices) {
        cout << vec[idx] << " ";
    }
    cout << endl;
    
    
    // fill the empty positions with randomized choice of 1 or 2 by shuffling the indices
    
    this_timer.reset();
    
    shuffle(empty_indices.begin(), empty_indices.end(), randgen);
    
    ctr = 0;
    for (const auto & idx : empty_indices) {
        vec[idx] = alternating[ctr];
        ctr++;
    }
    
    this_timer.stop();
    cout << "\nFilling the empty positions of vec took " << this_timer.ticks() << " seconds\n";

    
    // test the approach of assigning randomly one position at a time
    
    // initialize the vector with randomized hex game markers: 0->empty, 1, 2
    for (auto& elem : vec) {
        elem = rand() % 3;
    }
    
//  for (auto it = vec.begin(); it != vec.begin() + 60;  it++ ) {
//      *it = 0;
//  }
    
    this_timer.reset();
    
    for (auto& elem : vec) {
        if (elem == 0) {
            elem = rand() % 2 + 1;
        }
    }

    this_timer.stop();
    
    cout << "\nFilling one by one using random calculation took: " << this_timer.ticks() << "seconds\n";
    
    
//  // partition the non-empty positions to the front
//  stable_partition(vec.begin(), vec.end(), [](int p){return p!=0;});
//  
//  int ctr = 0;
//  for (auto& elem : vec) {
//      ctr++;
//      cout << elem << " ";
//      if (ctr % 10 == 0) 
//          cout << endl;
//      
//  }

    return 0;
}

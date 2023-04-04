#include <iostream>
#include <vector>
#include <random>
using namespace std;

class graph {
public:
    graph(int hsize);
    int who_won();
private:
    vector<vector <edge>> g;
    int size;
    vector<int> stone; // 0=empty, 1=side 1, 2=side 2
    
};

int main(int argc, char *argv[]) {
    
    
    return 0;
}
#include <iostream>
using namespace std;

int edge_len = 9;

int linear_index(int rank, int col)
{
    rank -= 1; // convert from input 1-based indexing to zero-based indexing
    col -= 1;
    if (rank < edge_len && col < edge_len) {
        return (rank * edge_len) + col;
    }
    else {
        cout << "Error: rank or col >= edge length\n";
        exit(-1);
    }
}

vector<int> linear_border(string & type, int side) {  // type in ["start", "finish"], side in [1,2]
    if (type == "start") {
        if (side == 1) {
            
        }
        else {
            
        }
    }
    else if (type == "finish") {
        if (side == 1) {
            
        }
        else {
            
        }
    }
    else {
        cout << "Error: type must be start or finish, got " << type << "/";
        exit(-1);
    }
}


int main(int argc, char *argv[]) {
    
    auto do_gen = []{  int rank=1; static int col=0; 
                      col++;
                      return linear_index(rank, col);
                    };
                    
    for (int i=0; i<edge_len; i++)
        cout << do_gen() << endl;
    
    return 0;
}
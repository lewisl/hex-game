#include <iostream>
#include <random>
using namespace std;


mt19937 rng(std::random_device());



// simple way to time segments of code
// ex:
//      Timing this_timer;
//      this_timer.reset();    // re-uses this_timer and starts it at a new time
//      < bunch-o-code >
//      this_timer.stop();
//      cout << "This code took " << this_timer.ticks() << " seconds\n";  
struct Timing {
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    double duration = 0.0;
    
    void reset() { start = std::chrono::steady_clock::now(); }
    
    void stop() { end = std::chrono::steady_clock::now(); }
    
    double ticks() { return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count(); }
};



void fill_alternating(vector<int> & vec, int start_idx, int end_idx) 
{
    int place = 2;
    int changer = 1;
    
    for (int i = start_idx; i < end_idx + 1; ++i) {
        changer *= -1;
        place += changer;
        vec[i] = place;
    }
}

// stuff to_vec with values in from_vec, optionally randomizing from_vec
void stuff(vector<int> & to_vec, vector<int> from_vec, int start_idx, int end_idx, bool randomize=false)
{
    int fills = end_idx - start_idx;
    int from_length = from_vec.size();
    if (randomize) 
        shuffle(from_vec.begin() + start_idx, from_vec.begin() + end_idx, rng);
    for (int i = start_idx, j = 0; i < end_idx + 1; ++i, ++j) {
        if (j >= from_length) {
            to_vec[i] = from_vec.back();
        }
        else
            to_vec[i] = from_vec[j];
    }
}

// stuff to_vec with a single value from start_idx to end_idx
void stuff(vector<int> & to_vec, int val, int start_idx, int end_idx)
{
    for (int i = start_idx; i < end_idx + 1; ++i) {
            to_vec[i] = val;
    }
}


int main(int argc, char *argv[]) {
    int size = 0;
    int iterations = 0;
    
    cout << "enter size: ";
    cin >> size;
    
    cout << "enter iterations ";
    cin >> iterations;

    vector<int> all_nodes(size, 0);
    vector<int> alternating(size, 0);
    
    fill_alternating(alternating, 0, size-1);
    
    Timing timer;
    
    timer.reset();
    
    for (int i = 0; i < iterations; i++) {
        stuff(all_nodes, alternating, 30, size-1, true);
        
        //  for (auto i : all_nodes)
        //      cout << i << endl;
        //  cout << endl;
        
        stuff(all_nodes, 1, 30, size-1);
    }
    
    timer.stop();
    cout << "time was " << timer.ticks() << endl;
    
    stuff(all_nodes, alternating, 30, size-1, true);
    
    for (auto i : all_nodes)
        cout << i << endl;
    cout << endl;

    
    return 0;
}
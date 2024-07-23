#ifndef HELPERS
#define HELPERS

/*
Helper functions and overrides:
- printing unordered_map
- printing deque
- printing vector

String helpers

an is_in function for simple linear search of several types of small containers

timing functions to measure execution time of running code and return it

*/



#include <deque> // sequence of nodes in a path between start and destination
#include <iostream>
#include <sstream> // to use stringstream to parse inputs from file
#include <stdlib.h> // for atoi()
#include <unordered_map> // container for definition of Graph
#include <vector>

using namespace std;

// send control codes to console to clear the screen
// not guaranteed to work on all OS'es.  Works on MacOs.
void clear_screen() { cout << u8"\033[2J"; }

// print key and values of an unordered_map
template <class T> ostream &operator<<(ostream &os, const unordered_map<int, int, T> &um)
{
    int count = 0;
    for (const auto &p : um) {
        os << "    key: " << p.first << " value: " << p.second;
        count++;
        if (count % 4 == 0)
            os << endl;
    }
    return os;
}

// print elements of a deque for various element types
template <class T> ostream &operator<<(ostream &os, const deque<T> &dq)
{
    int count = 0;
    for (const auto &p : dq) {
        os << " value: " << p;
        count++;
        if (count % 8 == 0)
            os << endl;
    }
    return os;
}

// print elements of a vector for various element types
template <class T> ostream &operator<<(ostream &os, const vector<T> &dq)
{
    int count = 0;
    for (const auto &p : dq) {
        os << " value: " << p;
        count++;
        if (count % 8 == 0)
            os << endl;
    }
    return os;
}

// SOME STRING HELPERS FOR DRAWING THE BOARD
// catenate multiple copies of a string
string string_by_n(string s, int n)
{
    string ret;
    for (int i = 0; i != n; ++i) {
        ret += s;
    }
    return ret;
}

std::string tolower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return tolower(c); });
    return result;
}

// other non-class functions

// test if value is in vector with trivial linear search for various primitive element types
template <typename T> bool is_in(T val, vector<T> vec)
{
    auto it = find(vec.cbegin(), vec.cend(), val);
    return it != vec.cend();
}

// test if value of char or string is in a string
template <typename T> // should be char or string
bool is_in(T val, const string &test_string)
{

    return (test_string.find(val) != std::string::npos);
}

// test if value is in deque with trivial linear search
template <typename T> bool is_in(T val, deque<T> deq)
{
    auto it = find(deq.cbegin(), deq.cend(), val);
    return it != deq.cend();
}

// compare to a single value not wrapped in a container
template <typename T> bool is_in(T val, T one) { return val == one; }

/** class Timing
a simple way to time the execution of segments of code
ex:
      Timing this_timer;
      this_timer.start();  // starts this_timer and saves the time
      < bunch-o-code >
      this_timer.cum();    // stops this_timer and adds the time since start to cumulative duration of this_timer
      cout << "This code took " << this_timer.show() << " seconds\n";

      other methods:
      this_timer.stop();   // stops this_timer and saves the time: used by cum
      this_timer.ticks();  // returns the time between the previous start and stop: used by cum
      this_timer.reset();  // resets start, stop, and duration (duration is updated and returned by cum),
                           // so that you can re-use this_timer.  You could also re-initialize it: Timing this_timer;
*/
class Timing {
  private:
    std::chrono::time_point<std::chrono::steady_clock> begint;
    std::chrono::time_point<std::chrono::steady_clock> endt;
    double duration = 0.0;

  public:
    void start() { begint = std::chrono::steady_clock::now(); }

    void stop() { endt = std::chrono::steady_clock::now(); }

    double ticks()
    {
        if (endt > begint)
            return chrono::duration_cast<std::chrono::duration<double>>(endt - begint).count();
        else
            return 0.0;
    }

    void cum()
    {
        stop();
        duration += ticks();
    }

    void reset()
    {
        std::chrono::time_point<std::chrono::steady_clock> begint;
        std::chrono::time_point<std::chrono::steady_clock> endt;
        double duration = 0.0;
    }

    double show() { return duration; }
};

#endif
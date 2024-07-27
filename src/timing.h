#ifndef TIMING_H
#define TIMING_H

/** class Timing
a simple way to time the execution of segments of code
ex:
      Timing this_timer;
      this_timer.start();  // starts this_timer and saves the time
      < bunch-o-code >
      this_timer.cum();    // stops this_timer and adds the time since start to
cumulative duration of this_timer cout << "This code took " << this_timer.show()
<< " seconds\n";

      other methods:
      this_timer.stop();   // stops this_timer and saves the time: used by cum
      this_timer.ticks();  // returns the time between the previous start and
stop: used by cum this_timer.reset();  // resets start, stop, and duration
(duration is updated and returned by cum),
                           // so that you can re-use this_timer.  You could also
re-initialize it: Timing this_timer;
*/

#include <chrono>
#include <ctime>

using namespace std;


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

    void reset()  // uncomment to use reset function
    {
        std::chrono::time_point<std::chrono::steady_clock> begint;
        std::chrono::time_point<std::chrono::steady_clock> endt;
        // double duration = 0.0;
    }

    double show() { return duration; }
};

#endif
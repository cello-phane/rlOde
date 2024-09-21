#pragma once
#if defined(_WIN32)
#include <time.h>
#include <windows.h>
#endif
#include <sys/time.h>
#include <tuple>
#include <chrono>
#include <thread>
//In windows, use the methods QueryPerformanceFrequency() and QueryPerformanceCounter().
//In Linux, it can be implemented using the method gettimeofday().
//The time is stored as a double

class Timer {
public:
  /**
     Default constructor - resets the timer for the first time.
  */
  Timer();
  /**
     Default destructor - doesn't do much.
  */
  ~Timer();
  /**
     This method resets the starting time. All the time Ellapsed function calls will use the reset start time for their time evaluations.
  */

  // Public method to start the timer
  void startTimer();

  // Function to convert elapsed seconds into hours, minutes, and seconds
  std::tuple<int, int, int> getTimeComponents(double seconds);

  double timeElapsed(const std::chrono::time_point<std::chrono::system_clock>& start);
  /* double timeElapsed();  */

  // Store the start time when the timer is started
  std::chrono::time_point<std::chrono::system_clock> start;
};



#pragma once
#if defined(_WIN32) //windows
#include <time.h>
#include <windows.h>
#else //linux
#include <sys/time.h>
#endif
#include <tuple>
#include <chrono>


//An alternative option?
//In windows, use the methods QueryPerformanceFrequency() and QueryPerformanceCounter().
//In Linux, it can be implemented using the method gettimeofday().
//The time is stored as a double

class GameTimer {
public:
  /**
     Default constructor - resets the timer for the first time.
  */
  GameTimer();
  /**
     Default destructor - doesn't do much.
  */
  ~GameTimer();
  /**
     This method resets the starting time. All the time Ellapsed function calls will use the reset start time for their time evaluations.
  */

  // Public method to start the timer
  void startGameTimer();

  // Function to convert elapsed seconds into hours, minutes, and seconds
  std::tuple<int, int, int> getGameTimeComponents(double seconds);

  double gameTimeElapsed(const std::chrono::time_point<std::chrono::system_clock>& start);
  /* double timeElapsed();  */

  // Store the start time when the timer is started
  std::chrono::time_point<std::chrono::system_clock> start;
};

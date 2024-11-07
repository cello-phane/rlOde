#include <gameTimer.h>

GameTimer::GameTimer(){
  startGameTimer();
}


GameTimer::~GameTimer(){

}

// Start the timer by setting the start point
void GameTimer::startGameTimer() {
  start = std::chrono::system_clock::now();
}

std::tuple<int, int, int> GameTimer::getGameTimeComponents(double seconds) {
  // Convert seconds into hours, minutes, and remaining seconds
  int hours = static_cast<int>(seconds) / 3600;
  int minutes = (static_cast<int>(seconds) % 3600) / 60;
  int secs = static_cast<int>(seconds) % 60;

  // Return hours, minutes, and seconds as a tuple
  return std::make_tuple(hours, minutes, secs);
}
double GameTimer::gameTimeElapsed(const std::chrono::time_point<std::chrono::system_clock>& start) {
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsedSeconds = end - start;
  return elapsedSeconds.count();
}
// double GameTimer::gameTimeElapsed() {
//   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
//   std::chrono::duration<double> diff = end - start;
//   return diff.count();  // Elapsed time in seconds
// }

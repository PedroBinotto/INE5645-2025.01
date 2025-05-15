#include "artifacts.hpp"

barrier::barrier(size_t count) : threshold(count), count(count) {}

void barrier::wait() {
  std::unique_lock<std::mutex> lock(mutex);
  size_t current_gen = generation;

  if (--count == 0) {
    generation++;
    count = threshold;
    cv.notify_all();
  } else {
    cv.wait(lock, [this, current_gen] { return current_gen != generation; });
  }
}

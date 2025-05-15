#ifndef __ARTIFACTS_H__
#define __ARTIFACTS_H__

#include <condition_variable>
#include <cstdlib>
#include <optional>
#include <queue>

struct barrier {
private:
  std::mutex mutex;
  std::condition_variable cv;
  const size_t threshold;
  size_t count;
  size_t generation = 0;

public:
  explicit barrier(size_t count);
  void wait();
};

template <typename T> struct synchronizing_queue {
private:
  std::queue<T> elements;
  std::mutex mutex;
  std::condition_variable cv;

public:
  void push(const T &element) {
    std::lock_guard<std::mutex> lock(mutex);
    elements.push(element);
    cv.notify_one();
  }

  std::optional<T> pop() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return !elements.empty(); });

    if (elements.empty())
      return std::nullopt;

    T element = elements.front();
    elements.pop();
    return element;
  }

  size_t size() {
    std::lock_guard<std::mutex> lock(mutex);
    return elements.size();
  }

  bool empty() {
    std::lock_guard<std::mutex> lock(mutex);
    return elements.empty();
  }
};

#endif

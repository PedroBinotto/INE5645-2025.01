#ifndef __ARTIFACTS_H__
#define __ARTIFACTS_H__

#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

struct barrier {
private:
  std::mutex mutex;
  std::condition_variable cv;
  const size_t threshold;
  size_t count;
  size_t generation = 0;

public:
  explicit barrier(size_t count) : threshold(count), count(count) {}
  void wait() {
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
};

struct counting_semaphore {
private:
  std::mutex mutex;
  std::condition_variable cv;
  int count;

public:
  counting_semaphore(int initial_count) : count(initial_count) {}
  void acquire() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return count > 0; });
    count--;
  }
  void release() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      count++;
    }
    cv.notify_one();
  }
};

template <typename T> struct synchronizing_queue {
private:
  std::queue<T> elements;
  std::mutex mutex;
  std::condition_variable cv;

public:
  void push(const T &element) {
    {

      std::lock_guard<std::mutex> lock(mutex);
      elements.push(element);
    }
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

  std::optional<T> peek() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return !elements.empty(); });

    if (elements.empty())
      return std::nullopt;

    T element = elements.front();
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

template <typename T, typename U> struct observer_subject {
public:
  void subscribe(T event_id, std::function<void(const U &)> callback) {
    std::lock_guard<std::mutex> lock(m);
    subscriptions[event_id].push_back(std::move(callback));
  }

  void notify(T event_id, const U &event) {
    std::lock_guard<std::mutex> lock(m);
    std::vector<std::function<void(const U &)>> fs = subscriptions[event_id];
    for (std::function<void(const U &)> callback : fs) {
      callback(event);
    }
  }

private:
  std::unordered_map<T, std::vector<std::function<void(const U &)>>> subscriptions;
  std::mutex m;
};

#endif

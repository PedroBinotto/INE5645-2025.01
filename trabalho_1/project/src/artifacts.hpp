#ifndef __ARTIFACTS_H__
#define __ARTIFACTS_H__

#include <condition_variable>
#include <cstdlib>

typedef struct barrier {
private:
  std::mutex mutex;
  std::condition_variable cv;
  const size_t threshold;
  size_t count;
  size_t generation = 0;

public:
  explicit barrier(size_t count);
  void wait();
} barrier;

#endif

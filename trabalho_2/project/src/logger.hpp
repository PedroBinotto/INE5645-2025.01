#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <fstream>
#include <memory>
#include <sys/stat.h>

#define LOG_DIR "log"
#define LOG_EXT "log"

class ThreadSafeLogger {
protected:
  ThreadSafeLogger();

public:
  void log(std::string msg);
  static std::shared_ptr<ThreadSafeLogger> get_instance();
  ~ThreadSafeLogger();

private:
  static std::mutex mtx;
  static std::shared_ptr<ThreadSafeLogger> instance;
  std::ofstream logfile;
};

inline void thread_safe_log(const std::string &msg) {
  ThreadSafeLogger::get_instance()->log(msg);
}

#endif

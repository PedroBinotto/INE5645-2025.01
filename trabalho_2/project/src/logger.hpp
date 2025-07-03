#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "types.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#define LOG_DIR "log"

class ThreadSafeLogger {
protected:
  ThreadSafeLogger();

public:
  void log(std::string msg);
  static std::shared_ptr<ThreadSafeLogger> get_instance();
  ~ThreadSafeLogger();

private:
  static std::mutex mtx;
  static std::unique_ptr<ThreadSafeLogger> instance;
  std::ofstream logfile;
};

#endif

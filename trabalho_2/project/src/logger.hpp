#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "store.hpp"
#include <format>
#include <fstream>
#include <memory>
#include <sys/stat.h>

#define LOG_DIR "log"
#define LOG_EXT "log"

/* Thread-safe implementation of a singleton Logger class. Outputs messages to
 * `LOG_DIR` and `stdout`
 */
class ThreadSafeLogger {
protected:
  ThreadSafeLogger();

public:
  /* Logs `msg` to `stdout` and `LOG_DIR`
   */
  void log(std::string msg);

  /* Provides access to the application-wide singleton Logger instance
   */
  static std::shared_ptr<ThreadSafeLogger> get_instance();
  ~ThreadSafeLogger();

private:
  static std::mutex mtx;
  static std::shared_ptr<ThreadSafeLogger> instance;
  std::ofstream logfile;
};

/* Provides function API to perform a method call to the global
 * `ThreadSafeLogger` instance
 */
inline void thread_safe_log(const std::string &msg) {
  ThreadSafeLogger::get_instance()->log(msg);
}

/* Adds a process-rank identifier tag to the beggining of the message
 */
inline std::string identify_log_string(std::string input, int world_rank) {
  return std::format("(@proc {0}) ", world_rank) + input;
}

/* Combines the behaviour of `thread_safe_log` and `identify_log_string`;
 */
inline void thread_safe_log_with_id(const std::string &msg, int id) {
  thread_safe_log(identify_log_string(msg, id));
}

/* Combines the behaviour of `thread_safe_log` and `identify_log_string`; pulls
 * `world_rank` from `GlobalRegistry`
 */
inline void thread_safe_log_with_id(const std::string &msg) {
  thread_safe_log_with_id(msg, registry_get(GlobalRegistryIndex::WorldRank));
}

#endif

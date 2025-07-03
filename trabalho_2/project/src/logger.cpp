#include "logger.hpp"

/* Creates a directory from (relative) `path`)
 */
void create_directory(const std::string &path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    if (mkdir(path.c_str(), 0777) == 0)
      return;
  } else if (info.st_mode & S_IFDIR) {
    return;
  }
}

/* Returns UNIX-time epoch string
 */
std::string currentUnixTime() {
  return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count());
}

std::unique_ptr<ThreadSafeLogger> ThreadSafeLogger::instance{nullptr};

std::mutex ThreadSafeLogger::mtx;

void ThreadSafeLogger::log(std::string msg) {
  std::lock_guard<std::mutex> lock(mtx);
  std::string log_entry = std::format("[{0}] {1}", currentUnixTime(), msg);
  std::cout << log_entry << std::endl;
  logfile << log_entry << std::endl;
}

ThreadSafeLogger::ThreadSafeLogger() {
  std::string epoch = currentUnixTime();
  create_directory(LOG_DIR);
  std::string file =
      std::format("{0}/{1}_{2}.{3}", LOG_DIR, epoch, "output", "log");
  logfile = std::ofstream(file);
}

std::shared_ptr<ThreadSafeLogger> ThreadSafeLogger::get_instance() {
  std::lock_guard<std::mutex> lock(mtx);
  if (instance.get() == nullptr) {
    instance = std::unique_ptr<ThreadSafeLogger>(new ThreadSafeLogger());
  }
  return std::move(instance);
};

ThreadSafeLogger::~ThreadSafeLogger(void) { logfile.close(); }

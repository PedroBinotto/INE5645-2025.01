#include "logger.hpp"
#include "store.hpp"
#include <chrono>
#include <iostream>

/* Creates a directory from (relative) `path`)
 */
void create_directory(const std::string &path);

/* Returns UNIX-time epoch string
 */
std::string currentUnixTime();

std::shared_ptr<ThreadSafeLogger> ThreadSafeLogger::instance{nullptr};

std::mutex ThreadSafeLogger::mtx;

void ThreadSafeLogger::log(std::string msg) {
  if (registry_get(GlobalRegistryIndex::LogLevel) <= 0)
    return;

  std::lock_guard<std::mutex> lock(mtx);
  std::string log_entry = std::format("[{0}] {1}", currentUnixTime(), msg);
  std::cout << log_entry << std::endl;
  logfile << log_entry << std::endl;
}

ThreadSafeLogger::ThreadSafeLogger() {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  std::string epoch =
      std::to_string(registry->get(GlobalRegistryIndex::Timestamp));
  std::string rank =
      std::to_string(registry->get(GlobalRegistryIndex::WorldRank));
  std::string dir = std::format("{0}/{1}", LOG_DIR, epoch);

  create_directory(dir);

  std::string file = std::format("{0}/proc-{1}_output.{2}", dir, rank, LOG_EXT);
  logfile = std::ofstream(file);
}

std::shared_ptr<ThreadSafeLogger> ThreadSafeLogger::get_instance() {
  std::lock_guard<std::mutex> lock(mtx);
  if (instance.get() == nullptr) {
    instance = std::shared_ptr<ThreadSafeLogger>(new ThreadSafeLogger());
  }
  return instance;
};

ThreadSafeLogger::~ThreadSafeLogger(void) { logfile.close(); }

void create_directory(const std::string &path) {
  if (path.empty()) {
    return;
  }

  struct stat info;
  if (stat(path.c_str(), &info) == 0) {
    if (info.st_mode & S_IFDIR) {
      return;
    } else {
      throw std::runtime_error("Path exists but is not a directory: " + path);
    }
  }

  size_t pos = path.find_last_of("/\\");
  if (pos != std::string::npos) {
    create_directory(path.substr(0, pos));
  }

  if (mkdir(path.c_str(), 0777) != 0) {
    if (stat(path.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
      throw std::runtime_error("Failed to create directory");
    }
  }
}

std::string currentUnixTime() {
  return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count());
}

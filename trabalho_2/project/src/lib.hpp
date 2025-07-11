#ifndef __LIB_H__
#define __LIB_H__

#include "constants.hpp"
#include "logger.hpp"
#include "mpi.h"
#include "store.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstring>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <utility>

class IRepository {
public:
  virtual block read(int key) = 0;
  virtual void write(int key, block value) = 0;
  virtual std::map<int, block> dump() = 0;
};

/**
 * Wrapper class for the process-local memory-blocks - that is - the memory
 * blocks that are maintained by the local process.
 */
class LocalRepository : public IRepository {
public:
  LocalRepository(memory_map mem_map, int block_size, int world_rank);
  block read(int key) override;
  void write(int key, block value) override;
  std::map<int, block> dump() override;
  ~LocalRepository();

private:
  memory_map mem_map;
  std::map<int, block> blocks;
  std::shared_mutex mtx;
  int block_size;
};

inline LocalRepository::LocalRepository(memory_map mem_map, int block_size,
                                        int world_rank)
    : mem_map(mem_map), blocks(std::map<int, block>()), block_size(block_size) {
  for (int i : mem_map.at(world_rank)) {
    block b = std::make_shared<std::uint8_t[]>(block_size);
    std::fill_n(b.get(), block_size, 0);

    blocks.emplace(i, b);
  }
}

inline LocalRepository::~LocalRepository() = default;

/**
 * Read contents from block indexed by `key`
 */
inline block LocalRepository::read(int key) {
  std::shared_lock lock(mtx);
  thread_safe_log_with_id(std::format(
      "READ operation to block {0} called at local repository level", key));
  auto it = blocks.find(key);
  if (it == blocks.end())
    throw std::runtime_error("Bad index");

  block copy = std::make_shared<std::uint8_t[]>(block_size);
  std::copy_n(it->second.get(), block_size, copy.get());

  return copy;
}

/**
 * Write `value` to memory block identified by `key`
 */
inline void LocalRepository::write(int key, block value) {
  {
    std::unique_lock lock(mtx);
    thread_safe_log_with_id(std::format(
        "WRITE operation to block {0} called at local repository level", key));
    blocks[key] = value;
  }

  long timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  thread_safe_log_with_id(std::format(
      "Sending out update notification request for block {0}...", key));

  NotificationMessageBuffer message(key, timestamp);
  std::shared_ptr<uint8_t[]> data = encode_notification_message(message);

  int send_result = MPI_Send(
      data.get(), get_total_notification_message_buffer_size(),
      MPI_UNSIGNED_CHAR,
      get_broadcaster_proc_rank(registry_get(GlobalRegistryIndex::WorldSize)),
      MESSAGE_TAG_BLOCK_UPDATE_NOTIFICATION, MPI_COMM_WORLD);

  if (send_result != MPI_SUCCESS)
    throw std::runtime_error("Encountered unexpected exception at `handler` "
                             "level while attempting "
                             "to perform NOTIFICATION request");
}

/**
 * Export a static representation of the current stored state (for debug and
 * logging purposes)
 */
inline std::map<int, block> LocalRepository::dump() {
  std::shared_lock lock(mtx);
  std::map<int, block> copy;
  int block_size = registry_get(GlobalRegistryIndex::BlockSize);
  for (const auto &[key, ptr] : blocks) {
    block new_buf = std::make_shared<uint8_t[]>(block_size);
    std::memcpy(new_buf.get(), ptr.get(), block_size);
    copy[key] = new_buf;
  }

  return copy;
}

/**
 * Wrapper class for the remote memory-blocks - that is - the memory
 * blocks that are maintained by the other processes.
 */
class RemoteRepository : public IRepository {
public:
  RemoteRepository(memory_map mem_map, int block_size, int world_rank);
  block read(int key) override;
  void write(int key, block value) override;
  std::map<int, block> dump() override;
  void invalidate_cache(int key);
  ~RemoteRepository();

private:
  memory_map mem_map;
  std::map<int, std::pair<int, block>> blocks;
  std::shared_mutex mtx;
  int block_size;
  int world_rank;
};

inline RemoteRepository::RemoteRepository(memory_map mem_map, int block_size,
                                          int world_rank)
    : mem_map(mem_map), blocks(std::map<int, std::pair<int, block>>()),
      block_size(block_size), world_rank(world_rank) {
  for (int i = 0; i < mem_map.size(); i++) {
    if (i == world_rank)
      continue;

    for (int j : mem_map.at(i)) {
      block b;
      blocks.emplace(j, std::make_pair(i, b));
    }
  }
}

inline RemoteRepository::~RemoteRepository() = default;

/**
 * Read contents from block indexed by `key`
 */
inline block RemoteRepository::read(int key) {
  std::shared_lock lock(mtx);
  thread_safe_log_with_id(std::format(
      "READ operation to block {0} called at remote repository level", key));
  auto handle_error = [&](const int &result, const std::string &type) {
    if (result != MPI_SUCCESS)
      throw std::runtime_error(identify_log_string(
          std::format("{0} failed with code: {1}", type, result), world_rank));
  };

  auto it = blocks.find(key);
  if (it == blocks.end())
    throw std::runtime_error("Bad index");

  std::pair<int, block> &block_pair = blocks.at(it->first);
  int target = block_pair.first;
  block &blk = block_pair.second;

  if (!blk) {
    thread_safe_log_with_id(
        std::format("Cached data not available for block {0}. Performing "
                    "remote access request...",
                    target));
    block buffer = std::make_shared<uint8_t[]>(block_size);

    handle_error(MPI_Send(&key, 1, MPI_INT, target,
                          MESSAGE_TAG_BLOCK_READ_REQUEST, MPI_COMM_WORLD),
                 "MPI_Send");

    thread_safe_log_with_id(
        std::format("Sent MPI request for block {0}", target));

    handle_error(MPI_Recv(buffer.get(), block_size, MPI_UNSIGNED_CHAR, target,
                          MESSAGE_TAG_BLOCK_READ_RESPONSE, MPI_COMM_WORLD,
                          MPI_STATUS_IGNORE),
                 "MPI_Recv");

    thread_safe_log_with_id(
        std::format("Received MPI response for block {0} with content {1}",
                    target, print_block(buffer)));

    blk = std::make_shared<uint8_t[]>(block_size);

    thread_safe_log_with_id(
        std::format("Saved local cache for block {0}", target));

    std::copy_n(buffer.get(), block_size, blk.get());
  } else {
    thread_safe_log_with_id(
        std::format("Using cached data for block {0}, contents: {1}", target,
                    print_block(blk)));
  }

  block copy = std::make_shared<std::uint8_t[]>(block_size);
  std::copy_n(blk.get(), block_size, copy.get());

  return copy;
}

/**
 * Write `value` to memory block identified by `key`
 */
inline void RemoteRepository::write(int key, block value) {
  std::unique_lock lock(mtx);
  thread_safe_log_with_id(std::format(
      "WRITE operation to block {0} called at remote repository level", key));
  int total_size = get_total_write_message_buffer_size();
  int target_maintainer = resolve_maintainer(key);

  thread_safe_log_with_id(
      std::format("Received method call to execute remote WRITE operation to "
                  "block {0} with value {1} on process ID {2}",
                  key, print_block(value), target_maintainer));

  WriteMessageBuffer buffer(key, value);

  std::shared_ptr<uint8_t[]> message_buffer = encode_write_message(buffer);

  thread_safe_log_with_id(std::format("Sending WRITE request of key: {0}, "
                                      "value: {1} serialized as {2} over MPI",
                                      buffer.key, print_block(buffer.data),
                                      print_block(message_buffer, total_size)));

  MPI_Send(message_buffer.get(), total_size, MPI_UNSIGNED_CHAR,
           target_maintainer, MESSAGE_TAG_BLOCK_WRITE_REQUEST, MPI_COMM_WORLD);
}

/**
 * Export a static representation of the current stored state (for debug and
 * logging purposes)
 */
inline std::map<int, block> RemoteRepository::dump() {
  std::shared_lock lock(mtx);
  std::map<int, block> copy;
  int block_size = registry_get(GlobalRegistryIndex::BlockSize);
  for (const auto &[key, ptr] : blocks) {
    if (ptr.second == nullptr) {
      copy[key] = nullptr;
    } else {
      block new_buf = std::make_shared<uint8_t[]>(block_size);
      std::memcpy(new_buf.get(), ptr.second.get(), block_size);
      copy[key] = new_buf;
    }
  }

  return copy;
}

/**
 * Clear locally cached data for block identified by `key`
 */
inline void RemoteRepository::invalidate_cache(int key) {
  std::unique_lock lock(mtx);
  thread_safe_log_with_id(
      std::format("Erasing local cache for block {0}", key));
  std::pair<int, block> &block_pair = blocks.at(key);
  block_pair.second = nullptr;
}

class UnifiedRepositoryFacade : public IRepository {
public:
  UnifiedRepositoryFacade(memory_map mem_map, int block_size, int world_rank);
  block read(int key) override;
  void write(int key, block value) override;
  void invalidate_cache(int key);
  std::map<int, block> dump() override;
  virtual ~UnifiedRepositoryFacade() = default;

private:
  std::map<int, std::shared_ptr<IRepository>> access_map;
  memory_map mem_map;
  std::shared_ptr<IRepository> local;
  std::shared_ptr<IRepository> remote;
};

inline UnifiedRepositoryFacade::UnifiedRepositoryFacade(memory_map mem_map,
                                                        int block_size,
                                                        int world_rank)
    : access_map(std::map<int, std::shared_ptr<IRepository>>()),
      mem_map(mem_map) {
  local = std::make_shared<LocalRepository>(mem_map, block_size, world_rank);
  remote = std::make_shared<RemoteRepository>(mem_map, block_size, world_rank);

  for (int i = 0; i < mem_map.size(); i++) {
    for (auto j : mem_map.at(i)) {
      access_map.emplace(j, i == world_rank ? local : remote);
    }
  }
};

/**
 * Write `value` to memory block identified by `key`
 */
inline void UnifiedRepositoryFacade::write(int key, block value) {
  access_map.at(key)->write(key, value);
}

/**
 * Read contents from block indexed by `key`
 */
inline block UnifiedRepositoryFacade::read(int key) {
  return access_map.at(key)->read(key);
}

/**
 * Export a static representation of the current stored state (for debug and
 * logging purposes)
 */
inline std::map<int, block> UnifiedRepositoryFacade::dump() {
  std::map<int, block> l = local->dump();
  std::map<int, block> r = remote->dump();
  l.merge(r);

  return l;
}

/**
 * Clear locally cached data for block identified by `key` (remote only)
 */
inline void UnifiedRepositoryFacade::invalidate_cache(int key) {
  std::vector<int> local_blocks =
      mem_map.at(registry_get(GlobalRegistryIndex::WorldRank));
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  if (local_set.contains(key))
    throw std::runtime_error("Bad index");

  static_cast<RemoteRepository &>(*access_map.at(key)).invalidate_cache(key);
}

#endif

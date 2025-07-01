#ifndef __LIB_H__
#define __LIB_H__

#include "mpi.h"
#include "ompi/mpi/cxx/mpicxx.h"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <map>
#include <utility>

class IRepository {
public:
  virtual block read(int key) = 0;
  virtual void write(int key, block value) = 0;
};

/* Wrapper class for the process-local memory-blocks - that is - the memory
 * blocks that are maintained by the local process.
 */
class LocalRepository : public IRepository {
public:
  LocalRepository(memory_map mem_map, int block_size, int world_rank);
  block read(int key) override;
  void write(int key, block value) override;
  ~LocalRepository();

private:
  memory_map mem_map;
  std::map<int, block> blocks;
  int block_size;
};

inline LocalRepository::LocalRepository(memory_map mem_map, int block_size,
                                        int world_rank)
    : mem_map(mem_map), blocks(std::map<int, block>()), block_size(block_size) {
  for (int i : mem_map.at(world_rank)) {
    block b = std::make_unique<std::uint8_t[]>(block_size);
    std::fill_n(b.get(), block_size, 0);
    blocks.emplace(i, std::move(b));
  }
}

inline LocalRepository::~LocalRepository() = default;

/* Read contents from block indexed by `key`
 */
inline block LocalRepository::read(int key) {
  auto it = blocks.find(key);
  if (it == blocks.end())
    throw std::runtime_error("bad index");

  block copy = std::make_unique<std::uint8_t[]>(block_size);
  std::copy_n(it->second.get(), block_size, copy.get());

  return copy;
}

/* Write `value` to memory block identified by `key` */
inline void LocalRepository::write(int key, block value) {
  throw std::runtime_error("not yet implemented");
}

/* Wrapper class for the remote memory-blocks - that is - the memory
 * blocks that are maintained by the other processes.
 */
class RemoteRepository : public IRepository {
public:
  RemoteRepository(memory_map mem_map, int block_size, int world_rank);
  block read(int key) override;
  void write(int key, block value) override;
  ~RemoteRepository();

private:
  memory_map mem_map;
  std::map<int, std::pair<int, block>> blocks;
  int block_size;
};

inline RemoteRepository::RemoteRepository(memory_map mem_map, int block_size,
                                          int world_rank)
    : mem_map(mem_map), blocks(std::map<int, std::pair<int, block>>()),
      block_size(block_size) {
  for (int i = 0; i < mem_map.size(); i++) {
    if (i == world_rank)
      continue;

    for (int j : mem_map.at(i)) {
      std::unique_ptr<uint8_t[]> block;
      blocks.emplace(j, std::make_pair(i, std::move(block)));
    }
  }
}

inline RemoteRepository::~RemoteRepository() = default;

/* Read contents from block indexed by `key`
 */
inline block RemoteRepository::read(int key) {
  auto it = blocks.find(key);
  if (it == blocks.end())
    throw std::runtime_error("bad index");

  std::pair<int, block> &block_pair = blocks.at(it->first);
  int target = block_pair.first;
  block &blk = block_pair.second;

  if (!blk) {
    uint8_t *buffer = (uint8_t *)malloc(block_size);

    MPI_Send(&key, 1, MPI::INT, target, MESSAGE_TAG_REQUEST, MPI_COMM_WORLD);

    MPI_Recv(buffer, block_size, MPI_UNSIGNED_CHAR, target, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);

    blk = std::make_unique<uint8_t[]>(block_size);
    std::copy_n(buffer, block_size, blk.get());

    free(buffer);
  }

  block copy = std::make_unique<std::uint8_t[]>(block_size);
  std::copy_n(blk.get(), block_size, copy.get());

  return copy;
}

/* Write `value` to memory block identified by `key` */
inline void RemoteRepository::write(int key, block value) {
  throw std::runtime_error("writing to remote is not allowed");
}

class UnifiedRepositoryFacade : public IRepository {
public:
  UnifiedRepositoryFacade(memory_map mem_map, int block_size, int world_rank);
  block read(int key) override;
  void write(int key, block value) override;
  ~UnifiedRepositoryFacade();

private:
  std::map<int, std::shared_ptr<IRepository>> access_map;
};

inline UnifiedRepositoryFacade::UnifiedRepositoryFacade(memory_map mem_map,
                                                        int block_size,
                                                        int world_rank)
    : access_map(std::map<int, std::shared_ptr<IRepository>>()) {
  std::shared_ptr<IRepository> local =
      std::make_shared<LocalRepository>(mem_map, block_size, world_rank);
  std::shared_ptr<IRepository> remote =
      std::make_shared<RemoteRepository>(mem_map, block_size, world_rank);

  for (int i = 0; i < mem_map.size(); i++) {
    for (auto j : mem_map.at(i)) {
      access_map.emplace(j, i == world_rank ? local : remote);
    }
  }
};

inline UnifiedRepositoryFacade::~UnifiedRepositoryFacade() = default;

/* Write `value` to memory block identified by `key` */
inline void UnifiedRepositoryFacade::write(int key, block value) {
  throw std::runtime_error("not yet implemented");
}

/* Read contents from block indexed by `key`
 */
inline block UnifiedRepositoryFacade::read(int key) {
  throw std::runtime_error("not yet implemented");
}

#endif

#ifndef __LIB_H__
#define __LIB_H__

#include "logger.hpp"
#include "mpi.h"
#include "ompi/mpi/cxx/mpicxx.h"
#include "store.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstring>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

class IRepository
{
  public:
    virtual block read(int key) = 0;
    virtual void write(int key, block value) = 0;
};

/* Wrapper class for the process-local memory-blocks - that is - the memory
 * blocks that are maintained by the local process.
 */
class LocalRepository : public IRepository
{
  public:
    LocalRepository(memory_map mem_map, int block_size, int world_rank);
    block read(int key) override;
    void write(int key, block value) override;
    ~LocalRepository();

  private:
    memory_map mem_map;
    std::map<int, block> blocks;
    std::shared_mutex mtx;
    int block_size;
};

inline LocalRepository::LocalRepository(memory_map mem_map, int block_size, int world_rank)
    : mem_map(mem_map), blocks(std::map<int, block>()), block_size(block_size)
{
    for (int i : mem_map.at(world_rank))
    {
        block b = std::make_unique<std::uint8_t[]>(block_size);
        std::fill_n(b.get(), block_size, 0);

        if (is_verbose(world_rank))
        { // TODO: add actual info
            thread_safe_log(identify_log_string(std::format("LocalRepository[{0}]: {1}", i, print_block(b, block_size)),
                                                world_rank));
        }

        blocks.emplace(i, std::move(b));
    }
}

inline LocalRepository::~LocalRepository() = default;

/* Read contents from block indexed by `key`
 */
inline block LocalRepository::read(int key)
{
    std::shared_lock lock(mtx);
    auto it = blocks.find(key);
    if (it == blocks.end())
        throw std::runtime_error("bad index");

    block copy = std::make_unique<std::uint8_t[]>(block_size);
    std::copy_n(it->second.get(), block_size, copy.get());

    return copy;
}

/* Write `value` to memory block identified by `key` */
inline void LocalRepository::write(int key, block value)
{
    std::unique_lock lock(mtx);
    blocks.emplace(key, std::move(value));
}

/* Wrapper class for the remote memory-blocks - that is - the memory
 * blocks that are maintained by the other processes.
 */
class RemoteRepository : public IRepository
{
  public:
    RemoteRepository(memory_map mem_map, int block_size, int world_rank);
    block read(int key) override;
    void write(int key, block value) override;
    ~RemoteRepository();

  private:
    memory_map mem_map;
    std::map<int, std::pair<int, block>> blocks;
    int block_size;
    int world_rank;
};

inline RemoteRepository::RemoteRepository(memory_map mem_map, int block_size, int world_rank)
    : mem_map(mem_map), blocks(std::map<int, std::pair<int, block>>()), block_size(block_size), world_rank(world_rank)
{
    for (int i = 0; i < mem_map.size(); i++)
    {
        if (i == world_rank)
            continue;

        for (int j : mem_map.at(i))
        {
            std::unique_ptr<uint8_t[]> block;

            if (is_verbose(world_rank))
            { // TODO: add actual info
                thread_safe_log(identify_log_string(std::format("RemoteRepository[{0}]: ", j), world_rank));
                if (block)
                {
                    thread_safe_log(print_block(block, block_size));
                }
                else
                {
                    thread_safe_log(identify_log_string("Empty block!", world_rank));
                }
            }

            blocks.emplace(j, std::make_pair(i, std::move(block)));
        }
    }
}

inline RemoteRepository::~RemoteRepository() = default;

/* Read contents from block indexed by `key`
 */
inline block RemoteRepository::read(int key)
{
    auto handle_error = [&](const int& result, const std::string& type)
    {
        if (result != MPI_SUCCESS)
            throw std::runtime_error(
                identify_log_string(std::format("{0} failed with code: {1}", type, result), world_rank));
    };

    auto it = blocks.find(key);
    if (it == blocks.end())
        throw std::runtime_error("bad index");

    std::pair<int, block>& block_pair = blocks.at(it->first);
    int target = block_pair.first;
    block& blk = block_pair.second;

    if (!blk)
    {
        block buffer = std::make_unique<uint8_t[]>(block_size);

        handle_error(MPI_Send(&key, 1, MPI::INT, target, MESSAGE_TAG_BLOCK_READ_REQUEST, MPI_COMM_WORLD), "MPI_Send");
        handle_error(MPI_Recv(buffer.get(),
                              block_size,
                              MPI_UNSIGNED_CHAR,
                              target,
                              MESSAGE_TAG_BLOCK_READ_RESPONSE,
                              MPI_COMM_WORLD,
                              MPI_STATUS_IGNORE),
                     "MPI_Recv");

        blk = std::make_unique<uint8_t[]>(block_size);
        std::copy_n(buffer.get(), block_size, blk.get());
    }

    block copy = std::make_unique<std::uint8_t[]>(block_size);
    std::copy_n(blk.get(), block_size, copy.get());

    return copy;
}

/* Write `value` to memory block identified by `key` */
inline void RemoteRepository::write(int key, block value)
{
    std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
    int world_size = registry->get(GlobalRegistryIndex::WorldSize);
    int total_size = get_total_write_message_size();

    std::unique_ptr<uint8_t[]> message_buffer = encode_write_message(key, std::move(value));

    MPI_Send(&message_buffer,
             total_size,
             MPI_UNSIGNED_CHAR,
             resolve_maintainer(world_size, key),
             MESSAGE_TAG_BLOCK_WRITE_REQUEST,
             MPI_COMM_WORLD);
}

class UnifiedRepositoryFacade : public IRepository
{
  public:
    UnifiedRepositoryFacade(memory_map mem_map, int block_size, int world_rank);
    block read(int key) override;
    void write(int key, block value) override;
    ~UnifiedRepositoryFacade();

  private:
    std::map<int, std::shared_ptr<IRepository>> access_map;
};

inline UnifiedRepositoryFacade::UnifiedRepositoryFacade(memory_map mem_map, int block_size, int world_rank)
    : access_map(std::map<int, std::shared_ptr<IRepository>>())
{
    std::shared_ptr<IRepository> local = std::make_shared<LocalRepository>(mem_map, block_size, world_rank);
    std::shared_ptr<IRepository> remote = std::make_shared<RemoteRepository>(mem_map, block_size, world_rank);

    for (int i = 0; i < mem_map.size(); i++)
    {
        for (auto j : mem_map.at(i))
        {
            access_map.emplace(j, i == world_rank ? local : remote);
        }
    }
};

inline UnifiedRepositoryFacade::~UnifiedRepositoryFacade() = default;

/* Write `value` to memory block identified by `key` */
inline void UnifiedRepositoryFacade::write(int key, block value) { access_map.at(key)->write(key, std::move(value)); }

/* Read contents from block indexed by `key`
 */
inline block UnifiedRepositoryFacade::read(int key) { return access_map.at(key)->read(key); }

#endif

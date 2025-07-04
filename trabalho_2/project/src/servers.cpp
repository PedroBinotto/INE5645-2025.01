#include "servers.hpp"
#include "lib.hpp"
#include "store.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <mpi.h>
#include <set>
#include <thread>
#include <unistd.h>

void handle_read(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                 int source);

void handle_write(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                  int source);

void handle_notify(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                   int source);

void read_listener(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);

  std::vector<int> local_blocks = mem_map.at(world_rank);
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  while (true) {
    int flag, probe_result;
    MPI_Status status;

    probe_result = MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_BLOCK_READ_REQUEST,
                              MPI_COMM_WORLD, &flag, &status);

    if (probe_result == MPI_SUCCESS && flag)
      handle_read(local_set, repo, status.MPI_SOURCE);

    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void write_listener(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);

  std::vector<int> local_blocks = mem_map.at(world_rank);
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  while (true) {
    int flag, probe_result;
    MPI_Status status;

    probe_result = MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_BLOCK_WRITE_REQUEST,
                              MPI_COMM_WORLD, &flag, &status);

    if (probe_result == MPI_SUCCESS && flag)
      handle_write(local_set, repo, status.MPI_SOURCE);

    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void handle_read(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                 int source) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);
  int block_size = registry->get(GlobalRegistryIndex::BlockSize);

  int requested_block;

  int recv_result = MPI_Recv(&requested_block, 1, MPI_INT, source,
                             MESSAGE_TAG_BLOCK_READ_REQUEST, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

  thread_safe_log(identify_log_string(
      std::format("received request for block {0}", requested_block),
      world_rank));

  if (recv_result != MPI_SUCCESS)
    throw std::runtime_error("failed to receive request");

  if (!local_blocks.contains(requested_block))
    throw std::runtime_error(
        "requested block is not maintained by this instance");
  try {
    block data = repo.read(requested_block);
    int send_result =
        MPI_Send(data.get(), block_size, MPI_UNSIGNED_CHAR, source,
                 MESSAGE_TAG_BLOCK_READ_RESPONSE, MPI_COMM_WORLD);

    if (send_result != MPI_SUCCESS)
      throw std::runtime_error("failed to send response");

  } catch (const std::exception &e) {
    throw std::runtime_error("error handling request");
  }
}

void handle_write(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                  int source) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);
  int block_size = registry->get(GlobalRegistryIndex::BlockSize);

  int target_block;
  block incoming_data = std::make_unique<std::uint8_t[]>(block_size);
  std::unique_ptr<uint8_t[]> result_buffer =
      std::make_unique<uint8_t[]>(get_total_write_message_size(block_size));

  int recv_result = MPI_Recv(&result_buffer, 1, MPI_INT, source,
                             MESSAGE_TAG_BLOCK_WRITE_REQUEST, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

  std::memcpy(&target_block, result_buffer.get(), sizeof(int));
  std::memcpy(incoming_data.get(), result_buffer.get() + sizeof(int),
              block_size);

  thread_safe_log(identify_log_string(
      std::format("received write for block {0}", target_block), world_rank));

  if (recv_result != MPI_SUCCESS)
    throw std::runtime_error("failed to receive request");

  if (!local_blocks.contains(target_block))
    throw std::runtime_error(
        "requested block is not maintained by this instance");
  try {
    repo.write(target_block, std::move(incoming_data));

    int send_result =
        MPI_Send(data.get(), block_size, MPI_UNSIGNED_CHAR, source,
                 MESSAGE_TAG_BLOCK_READ_RESPONSE, MPI_COMM_WORLD);

    if (send_result != MPI_SUCCESS)
      throw std::runtime_error("failed to notify");

  } catch (const std::exception &e) {
    throw std::runtime_error("error handling request");
  }
}

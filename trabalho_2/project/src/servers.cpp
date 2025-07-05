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
#include <utility>

void handle_read(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                 int source);

void handle_write(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                  int source);

void handle_notify(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                   int source);

void read_listener(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);

  thread_safe_log(identify_log_string("Read thread started", world_rank));

  std::vector<int> local_blocks = mem_map.at(world_rank);
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  thread_safe_log(identify_log_string("Acquired memory", world_rank));

  while (true) {
    try {
      int flag, probe_result;
      MPI_Status status;

      probe_result = MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_BLOCK_READ_REQUEST,
                                MPI_COMM_WORLD, &flag, &status);
      thread_safe_log(identify_log_string("Probed", world_rank));

      if (probe_result == MPI_SUCCESS && flag) {
        thread_safe_log(identify_log_string("Received request", world_rank));
        handle_read(local_set, repo, status.MPI_SOURCE);
      }

      std::this_thread::sleep_for(std::chrono::microseconds(100));
    } catch (const std::exception &e) {
      thread_safe_log(identify_log_string(
          std::format("Caught exception {0}", e.what()), world_rank));
    }
  }

  thread_safe_log(identify_log_string("Broke out of loop", world_rank));
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
  thread_safe_log(identify_log_string("Hadling read", world_rank));

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
    thread_safe_log(identify_log_string("inside try block", world_rank));
    block data = repo.read(requested_block);
    int send_result =
        MPI_Send(data.get(), block_size, MPI_UNSIGNED_CHAR, source,
                 MESSAGE_TAG_BLOCK_READ_RESPONSE, MPI_COMM_WORLD);

    thread_safe_log(identify_log_string("sent block", world_rank));

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
  int total_size = get_total_write_message_buffer_size();

  std::unique_ptr<uint8_t[]> result_buffer =
      std::make_unique<uint8_t[]>(total_size);
  int recv_result = MPI_Recv(&result_buffer, 1, MPI_UNSIGNED_CHAR, source,
                             MESSAGE_TAG_BLOCK_WRITE_REQUEST, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

  std::unique_ptr<WriteMessageBuffer> message_buffer =
      decode_write_message(std::move(result_buffer));

  thread_safe_log(identify_log_string(
      std::format("received write for block {0}", message_buffer->target_block),
      world_rank));

  if (recv_result != MPI_SUCCESS)
    throw std::runtime_error("failed to receive request");

  if (!local_blocks.contains(message_buffer->target_block))
    throw std::runtime_error(
        "requested block is not maintained by this instance");
  try {
    repo.write(message_buffer->target_block,
               std::move(message_buffer->incoming_data));

    // TODO: Notify for cache invalidation
    // int send_result = MPI_Bcast(result_buffer, total_size, MPI_UNSIGNED_CHAR,
    // source, MPI_COMM_WORLD); if (send_result != MPI_SUCCESS)
    //   throw std::runtime_error("failed to notify");
  } catch (const std::exception &e) {
    throw std::runtime_error("error handling request");
  }
}

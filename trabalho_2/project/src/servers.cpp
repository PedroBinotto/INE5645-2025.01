#include "servers.hpp"
#include "constants.hpp"
#include "lib.hpp"
#include "logger.hpp"
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

void handle_notify(int source);

void read_listener(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  thread_safe_log_with_id("Read thread started");

  std::vector<int> local_blocks =
      mem_map.at(registry_get(GlobalRegistryIndex::WorldRank));
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  while (true) {
    thread_safe_log_with_id("Read listener probing...");

    int flag, probe_result;
    MPI_Status status;

    probe_result = MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_BLOCK_READ_REQUEST,
                              MPI_COMM_WORLD, &flag, &status);
    if (probe_result == MPI_SUCCESS && flag) {
      thread_safe_log_with_id(
          "Detected READ operation request at `listener` level");
      handle_read(local_set, repo, status.MPI_SOURCE);
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(OPERATION_SLEEP_INTERVAL_MILLIS));
  }
}

void write_listener(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  thread_safe_log_with_id("Write thread started");

  std::vector<int> local_blocks =
      mem_map.at(registry_get(GlobalRegistryIndex::WorldRank));
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  while (true) {
    thread_safe_log_with_id("Write listener probing...");

    int flag, probe_result;
    MPI_Status status;

    probe_result = MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_BLOCK_WRITE_REQUEST,
                              MPI_COMM_WORLD, &flag, &status);

    if (probe_result == MPI_SUCCESS && flag) {
      thread_safe_log_with_id(
          "Detected WRITE operation request at `listener` level");
      handle_write(local_set, repo, status.MPI_SOURCE);
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(OPERATION_SLEEP_INTERVAL_MILLIS));
  }
}

void notification_listener(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  thread_safe_log_with_id("Notification listener thread started");
  int total_size = get_total_notification_message_buffer_size();
  std::vector<int> local_blocks =
      mem_map.at(registry_get(GlobalRegistryIndex::WorldRank));
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  while (true) {
    thread_safe_log_with_id("Notification listener probing...");
    std::shared_ptr<uint8_t[]> result_buffer =
        std::make_shared<uint8_t[]>(total_size);
    MPI_Bcast(
        result_buffer.get(), total_size, MPI_UNSIGNED_CHAR,
        get_broadcaster_proc_rank(registry_get(GlobalRegistryIndex::WorldSize)),
        MPI_COMM_WORLD);

    thread_safe_log_with_id(
        std::format("Received notification broadcast with message m = {0}",
                    print_block(result_buffer, total_size)));
    NotificationMessageBuffer message =
        decode_notificaton_message(result_buffer);

    if (local_set.contains(message.key)) {
      thread_safe_log_with_id(
          "Skipping update notification for locally-maintained block...");
      continue;
    }

    thread_safe_log_with_id(std::format(
        "Requesting cache invalidation for block {0}", message.key));
    repo.invalidate_cache(message.key);
  }
}

void notification_broadcaster() {
  thread_safe_log_with_id("Notification broadcaster server started");

  while (true) {
    thread_safe_log_with_id("Notification broadcaster probing...");

    int flag, probe_result;
    MPI_Status status;

    probe_result =
        MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_BLOCK_UPDATE_NOTIFICATION,
                   MPI_COMM_WORLD, &flag, &status);

    if (probe_result == MPI_SUCCESS && flag) {
      thread_safe_log_with_id(
          "Detected NOTIFIATON operation request at `listener` level");
      handle_notify(status.MPI_SOURCE);
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(OPERATION_SLEEP_INTERVAL_MILLIS));
  }
}

void handle_read(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                 int source) {
  thread_safe_log_with_id(
      "Processing READ operation request at `handler` level...");

  int requested_block;

  int recv_result = MPI_Recv(&requested_block, 1, MPI_INT, source,
                             MESSAGE_TAG_BLOCK_READ_REQUEST, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

  if (recv_result != MPI_SUCCESS)
    throw std::runtime_error(
        "MPI error while processing READ request at `handler` level");

  thread_safe_log_with_id(
      std::format("Successfully interpreted at `handler` level that targeted "
                  "block for READ operation is {0}",
                  requested_block));

  if (!local_blocks.contains(requested_block))
    throw std::runtime_error(
        "Targeted block for READ operation is not maintained by this instance");
  try {
    block data = repo.read(requested_block);
    thread_safe_log_with_id(std::format(
        "Completed READ request from process of ID {0} successfully. Sending "
        "out response for block {1}...",
        source, requested_block));

    int send_result =
        MPI_Send(data.get(), registry_get(GlobalRegistryIndex::BlockSize),
                 MPI_UNSIGNED_CHAR, source, MESSAGE_TAG_BLOCK_READ_RESPONSE,
                 MPI_COMM_WORLD);

    if (send_result != MPI_SUCCESS)
      throw std::runtime_error("MPI error while attempting to send response to "
                               "READ operation request at `handler` level");
  } catch (const std::exception &e) {
    throw std::runtime_error(
        "Encountered unexpected exception at `handler` level while attempting "
        "to process READ operation request");
  }
}

void handle_write(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                  int source) {
  int total_size = get_total_write_message_buffer_size();

  thread_safe_log_with_id(
      "Processing WRITE operation request at `handler` level...");

  std::shared_ptr<uint8_t[]> result_buffer =
      std::make_shared<uint8_t[]>(total_size);
  int recv_result = MPI_Recv(result_buffer.get(), total_size, MPI_UNSIGNED_CHAR,
                             source, MESSAGE_TAG_BLOCK_WRITE_REQUEST,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  if (recv_result != MPI_SUCCESS)
    throw std::runtime_error(
        "MPI error while processing WRITE request at `handler` level");

  thread_safe_log_with_id(
      std::format("Successfully interpreted at `handler` "
                  "level WRITE operation coming from process of ID {0}; "
                  "request with total buffer contents {1}",
                  source, print_block(result_buffer, total_size)));

  WriteMessageBuffer message_buffer = decode_write_message(result_buffer);

  if (!local_blocks.contains(message_buffer.key))
    throw std::runtime_error("Targeted block for WRITE operation is not "
                             "maintained by this instance");
  try {
    repo.write(message_buffer.key, message_buffer.data);
    thread_safe_log_with_id(
        "Completed WRITE request from process of ID {0} successfully.");
  } catch (const std::exception &e) {
    throw std::runtime_error(
        "Encountered unexpected exception at `handler` level while attempting "
        "to process WRITE operation request");
  }
}

void handle_notify(int source) {
  int total_size = get_total_notification_message_buffer_size();

  thread_safe_log_with_id(
      "Processing NOTIFICATION operation request at `handler` level...");

  std::shared_ptr<uint8_t[]> result_buffer =
      std::make_shared<uint8_t[]>(total_size);
  int recv_result = MPI_Recv(result_buffer.get(), total_size, MPI_UNSIGNED_CHAR,
                             source, MESSAGE_TAG_BLOCK_UPDATE_NOTIFICATION,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  if (recv_result != MPI_SUCCESS)
    throw std::runtime_error(
        "MPI error while processing NOTIFICATION request at `handler` level");

  thread_safe_log_with_id(
      std::format("Successfully interpreted at `handler` "
                  "level NOTIFICATION operation coming from process of ID {0}; "
                  "request with total buffer contents {1}",
                  source, print_block(result_buffer, total_size)));

  decode_notificaton_message(
      result_buffer); // DEBUG: Decoding to print out struct repr.

  int bcast_result = MPI_Bcast(
      result_buffer.get(), total_size, MPI_UNSIGNED_CHAR,
      get_broadcaster_proc_rank(registry_get(GlobalRegistryIndex::WorldSize)),
      MPI_COMM_WORLD);

  if (bcast_result != MPI_SUCCESS)
    throw std::runtime_error(
        "MPI error while attemption NOTIFICATION broadcast at `handler` level");

  thread_safe_log_with_id(std::format(
      "Successfully submitted broadcast message with total contents {0}",
      print_block(result_buffer, total_size)));
}

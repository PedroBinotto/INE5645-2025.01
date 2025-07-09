#include "lib.hpp"
#include "logger.hpp"
#include "servers.hpp"
#include "store.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <format>
#include <mpi.h>
#include <random>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <thread>
#include <tuple>
#include <unistd.h>

server_threads start_helper_treads(memory_map mem_map,
                                   UnifiedRepositoryFacade &repo);

int main(int argc, const char **argv) {
  std::mt19937 rng{std::random_device{}()};
  int world_size, world_rank, name_len, thread_safety_provided;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &thread_safety_provided);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Get_processor_name(processor_name, &name_len);

  if (thread_safety_provided < MPI_THREAD_MULTIPLE) {
    throw std::runtime_error("multithread not supported");
  }

  const bool verbose = is_verbose(world_rank);
  program_args params = capture_args(argc, argv, verbose);

  int timestamp = std::stoi(std::get<0>(params));
  int block_size = std::get<1>(params);
  int num_blocks = std::get<2>(params);

  validate_args(params, world_size, verbose);

  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance(
      world_rank, world_size, num_blocks, block_size, timestamp);
  memory_map mem_map = resolve_maintainers(world_size, num_blocks);
  UnifiedRepositoryFacade repo =
      UnifiedRepositoryFacade(mem_map, block_size, world_rank);

  server_threads threads = start_helper_treads(mem_map, std::ref(repo));

  MPI_Barrier(MPI_COMM_WORLD);

  thread_safe_log_with_id(std::format(
      "Hello, World! from processor {0}, rank {1} out of {2} processors",
      processor_name, world_rank, world_size));

  while (true) {
    int target_block = rng() % num_blocks;
    int operation = rng() % 2;

    // DEBUG
    if (operation) {
      block v = get_random_block();
      repo.write(target_block, v);
      thread_safe_log_with_id(
          std::format("Performing READ operation to block {0} at `main` level",
                      target_block));
    } else {
      thread_safe_log_with_id(
          std::format("Performing WRITE operation to block {0} at `main` level",
                      target_block));
    }
    // DEBUG

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5000 / (target_block + 1)));
  }

  std::apply([](auto &&...thread) { ((thread.join()), ...); }, threads);
  MPI_Finalize();
}

/* Starts all server threads and returns them `server_threads`:
 */
server_threads start_helper_treads(memory_map mem_map,
                                   UnifiedRepositoryFacade &repo) {
  return std::make_tuple(
      std::thread(read_listener, mem_map, std::ref(repo)),
      std::thread(write_listener, mem_map, std::ref(repo)),
      std::thread(notification_listener, mem_map, std::ref(repo)));
}

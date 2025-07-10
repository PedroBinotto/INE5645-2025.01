#include "constants.hpp"
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
#include <utility>

/* Starts all server threads and returns them `server_threads`:
 */
server_threads start_helper_treads(memory_map mem_map,
                                   UnifiedRepositoryFacade &repo);

/* Implements worker operations
 */
void worker_proc(memory_map mem_map, std::string processor_name, int block_size,
                 int num_blocks, int world_rank, int world_size);

/* Implements broadcaster operations
 */
void broadcaster_proc();

/* Helper function to register state changes
 */
std::string dump_current_state(UnifiedRepositoryFacade &repo);

int main(int argc, const char **argv) {
  int world_size, world_rank, name_len, thread_safety_provided;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &thread_safety_provided);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Get_processor_name(processor_name, &name_len);

  std::cout << "Process assigned world rank " << world_rank
            << " successfully initialized MPI" << std::endl;

  if (thread_safety_provided < MPI_THREAD_MULTIPLE) {
    throw std::runtime_error(
        "Multithread operation not supported. Aborting...");
  }

  const bool verbose = is_verbose(world_rank);
  program_args params = capture_args(argc, argv, verbose);

  std::cout << "Process assigned world rank " << world_rank
            << " successfully parsed program args" << std::endl;

  int log_level = std::get<0>(params);
  int timestamp = std::stoi(std::get<1>(params));
  int block_size = std::get<2>(params);
  int num_blocks = std::get<3>(params);

  validate_args(params, world_size, verbose);

  std::cout << "Process assigned world rank " << world_rank
            << " successfully validated program args" << std::endl;

  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance(
      world_rank, world_size, num_blocks, block_size, timestamp, log_level);
  memory_map mem_map = resolve_maintainers();

  if (world_rank == get_broadcaster_proc_rank(world_size)) {
    broadcaster_proc();
  } else {
    worker_proc(mem_map, processor_name, block_size, num_blocks, world_rank,
                world_size);
  }

  MPI_Finalize();
  return 0;
}

void worker_proc(memory_map mem_map, std::string processor_name, int block_size,
                 int num_blocks, int world_rank, int world_size) {
  thread_safe_log_with_id("Started as worker process");

  std::mt19937 rng{std::random_device{}()};
  UnifiedRepositoryFacade repo =
      UnifiedRepositoryFacade(mem_map, block_size, world_rank);

  server_threads threads = start_helper_treads(mem_map, std::ref(repo));

  thread_safe_log_with_id("Started helper threads");

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
      repo.read(target_block);
      thread_safe_log_with_id(
          std::format("Performing WRITE operation to block {0} at `main` level",
                      target_block));
    }
    // DEBUG

    std::this_thread::sleep_for(std::chrono::milliseconds(
        OPERATION_SLEEP_INTERVAL_MILLIS / (target_block + 1)));

    if (registry_get(GlobalRegistryIndex::LogLevel) > 1)
      thread_safe_log_with_id(
          std::format("DEBUG: Current local allocated block configuration: {0}",
                      dump_current_state(repo)));
  }

  std::apply([](auto &&...thread) { ((thread.join()), ...); }, threads);
}

void broadcaster_proc() {
  thread_safe_log_with_id("Started as notification broadcaster");

  std::thread t = std::thread(notification_broadcaster);
  MPI_Barrier(MPI_COMM_WORLD);
  t.join();
}

server_threads start_helper_treads(memory_map mem_map,
                                   UnifiedRepositoryFacade &repo) {
  return std::make_tuple(
      std::thread(read_listener, mem_map, std::ref(repo)),
      std::thread(write_listener, mem_map, std::ref(repo)),
      std::thread(notification_listener, mem_map, std::ref(repo)));
}

std::string dump_current_state(UnifiedRepositoryFacade &repo) {
  int num_blocks = registry_get(GlobalRegistryIndex::NumBlocks);
  std::string s;
  std::map<int, block> state = repo.dump();

  auto transform = [&](const std::pair<int, block> &p) {
    std::string blk = p.second ? print_block(p.second) : "nullptr";
    return std::format("{0}	{1}\n", p.first, blk);
  };

  for (int i = 0; i < num_blocks; i++) {
    s += transform(std::make_pair(i, state.at(i)));
  }

  return s;
}

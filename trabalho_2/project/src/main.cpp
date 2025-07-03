#include "lib.hpp"
#include "store.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <format>
#include <mpi.h>
#include <random>
#include <set>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>

void receive_request(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                     int source);
void server(memory_map mem_map, UnifiedRepositoryFacade &repo);

int main(int argc, const char **argv) {
  std::mt19937 rng{std::random_device{}()};
  int world_size, world_rank, name_len;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Get_processor_name(processor_name, &name_len);

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

  std::thread server_thread = std::thread(server, mem_map, std::ref(repo));

  MPI_Barrier(MPI_COMM_WORLD);

  thread_safe_log(std::format(
      "Hello, World! from processor {0}, rank {1} out of {2} processors",
      processor_name, world_rank, world_size));

  while (true) {
    int target_block = rng() % num_blocks;
    thread_safe_log(identify_log_string(
        std::format("Reading block {0}: {1}", target_block,
                    print_block(repo.read(target_block), block_size)),
        world_rank));
    ;

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5000 / (target_block + 1)));
  }

  MPI_Finalize();
}

void receive_request(std::set<int> &local_blocks, UnifiedRepositoryFacade &repo,
                     int source) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);
  int block_size = registry->get(GlobalRegistryIndex::BlockSize);

  int requested_block;

  int recv_result =
      MPI_Recv(&requested_block, 1, MPI_INT, source, MESSAGE_TAG_REQUEST,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
    int send_result = MPI_Send(data.get(), block_size, MPI_UNSIGNED_CHAR,
                               source, MESSAGE_TAG_RESPONSE, MPI_COMM_WORLD);

    if (send_result != MPI_SUCCESS)
      throw std::runtime_error("failed to send response");

  } catch (const std::exception &e) {
    throw std::runtime_error("error handling request for block");
  }
}

void server(memory_map mem_map, UnifiedRepositoryFacade &repo) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int world_rank = registry->get(GlobalRegistryIndex::WorldRank);

  std::vector<int> local_blocks = mem_map.at(world_rank);
  std::set<int> local_set = std::set(local_blocks.begin(), local_blocks.end());

  while (true) {
    int flag, probe_result;
    MPI_Status status;

    probe_result = MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_TAG_REQUEST,
                              MPI_COMM_WORLD, &flag, &status);

    if (probe_result == MPI_SUCCESS && flag)
      receive_request(local_set, repo, status.MPI_SOURCE);

    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

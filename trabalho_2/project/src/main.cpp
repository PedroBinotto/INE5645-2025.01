#include "constants.hpp"
#include "lib.hpp"
#include "logger.hpp"
#include "servers.hpp"
#include "store.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <format>
#include <memory>
#include <mpi.h>
#include <random>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <utility>

// *****************************************************************************
// FUNÇÕES ESPECIFICADAS NO ENUNCIADO DO TRABALHO

/**
 * Primitiva de escrita
 *
 * @param posicao Indica a posição inicial da memória de onde se pretende
 * escrever algum conteúdo;
 * @param buffer Indica o endereço da variável que contém o conteúdo a ser
 * escrito no espaço de endereçamento;
 * @param tamanho Indica o número em bytes a serem escritos na operação (ou
 * seja, o número de bytes em buffer, a partir da posição `posicao`);
 * @return valor de retorno inteiro (`int`) deve representar códigos de erro, na
 * impossibilidade de execução da operação.
 */
int escreve(int posicao, std::shared_ptr<uint8_t[]> buffer, int tamanho);

/**
 * Primitiva de leitura
 *
 * @param posicao Indica a posição inicial da memória de onde se pretende ler
 * algum conteúdo;
 * @param buffer Indica o endereço da variável que receberá o conteúdo da
 * leitura;
 * @param tamanho Indica o número em bytes a serem lidos na operação (ou seja, o
 * número de bytes a partir da posição `posicao`);
 * @return valor de retorno inteiro (`int`) deve representar códigos de erro, na
 * impossibilidade de execução da operação.
 */
int le(int posicao, std::shared_ptr<uint8_t[]> buffer, int tamanho);

// FUNÇÕES ESPECIFICADAS NO ENUNCIADO DO TRABALHO
// *****************************************************************************

/**
 * Starts all server threads and returns them `server_threads`:
 */
server_threads start_helper_threads(memory_map mem_map,
                                    UnifiedRepositoryFacade &repo);

/**
 * Implements worker operations
 */
void worker_proc(memory_map mem_map, std::string processor_name, int block_size,
                 int num_blocks, int world_rank, int world_size);

/**
 * Implements broadcaster operations
 */
void broadcaster_proc();

/**
 * Helper function to register state changes
 */
std::string dump_current_state(UnifiedRepositoryFacade &repo);

std::optional<UnifiedRepositoryFacade> repository;

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
  repository = UnifiedRepositoryFacade(mem_map, block_size, world_rank);

  server_threads threads =
      start_helper_threads(mem_map, std::ref(repository.value()));

  thread_safe_log_with_id("Started helper threads");

  MPI_Barrier(MPI_COMM_WORLD);

  thread_safe_log_with_id(std::format(
      "Hello, World! from processor {0}, rank {1} out of {2} processors",
      processor_name, world_rank, world_size));

  while (true) {
    int target_block = rng() % num_blocks;
    int size = rng() % ((num_blocks - target_block) * block_size);

    if (rng() % 2) {
      std::shared_ptr<uint8_t[]> buffer = get_random_block(size);
      escreve(target_block, buffer, size);
    } else {
      std::shared_ptr<uint8_t[]> result_buffer =
          std::make_shared<uint8_t[]>(size);
      le(target_block, result_buffer, size);
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(OPERATION_SLEEP_INTERVAL_MILLIS));

    if (registry_get(GlobalRegistryIndex::LogLevel) > 1)
      thread_safe_log_with_id(
          std::format("DEBUG: Current local allocated block configuration: {0}",
                      dump_current_state(repository.value())));
  }

  std::apply([](auto &&...thread) { ((thread.join()), ...); }, threads);
}

void broadcaster_proc() {
  thread_safe_log_with_id("Started as notification broadcaster");

  std::thread t = std::thread(notification_broadcaster);
  MPI_Barrier(MPI_COMM_WORLD);
  t.join();
}

server_threads start_helper_threads(memory_map mem_map,
                                    UnifiedRepositoryFacade &repo) {
  return std::make_tuple(
      std::thread(read_listener, mem_map, std::ref(repo)),
      std::thread(write_listener, mem_map, std::ref(repo)),
      std::thread(notification_listener, mem_map, std::ref(repo)));
}

std::string dump_current_state(UnifiedRepositoryFacade &repo) {
  int num_blocks = registry_get(GlobalRegistryIndex::NumBlocks);
  std::string s = "\n";
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

int escreve(int posicao, std::shared_ptr<uint8_t[]> buffer, int tamanho) {
  int num_blocks = registry_get(GlobalRegistryIndex::NumBlocks);
  int block_size = registry_get(GlobalRegistryIndex::BlockSize);
  int scoped_blocks = std::ceil(static_cast<double>(tamanho) / block_size);
  int final_pos = posicao + scoped_blocks;

  if (final_pos > num_blocks)
    return 1;

  for (int i = 0; i < scoped_blocks; i++) {
    block new_buf = std::make_shared<uint8_t[]>(block_size);
    std::memcpy(new_buf.get(), buffer.get() + (i * block_size), block_size);
    repository->write(posicao + i, new_buf);
    thread_safe_log_with_id(std::format(
        "Performing WRITE operation to block {0} at `main` level", posicao));
  }

  return 0;
}

int le(int posicao, std::shared_ptr<uint8_t[]> buffer, int tamanho) {
  int num_blocks = registry_get(GlobalRegistryIndex::NumBlocks);
  int block_size = registry_get(GlobalRegistryIndex::BlockSize);
  int scoped_blocks = std::ceil(static_cast<double>(tamanho) / block_size);
  int final_pos = posicao + scoped_blocks;

  if (final_pos > num_blocks)
    return 1;

  for (int i = 0; i < scoped_blocks; i++) {
    block result = repository->read(posicao + i);
    std::memcpy(buffer.get() + (i * block_size), result.get(), block_size);
    thread_safe_log_with_id(std::format(
        "Performing READ operation to block {0} at `main` level", posicao));
  }

  return 0;
}

#ifndef __UTILS_H__
#define __UTILS_H__

#include "store.hpp"
#include "types.hpp"
#include <bitset>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define DEFAULT_BLOCK_SIZE 8
#define DEFAULT_NUM_BLOCKS 4
#define MAX_BLOCK_SIZE 32
#define MAX_NUM_BLOCKS 32
#define MASTER_INSTANCE_ID 0

#define MESSAGE_TAG_BLOCK_READ_REQUEST 100
#define MESSAGE_TAG_BLOCK_READ_RESPONSE 101
#define MESSAGE_TAG_BLOCK_WRITE_REQUEST 102
#define MESSAGE_TAG_BLOCK_UPDATE_NOTIFICATION 103

/* Formats `std::vector<T>` to pretty-print friendly representation
 */
template <typename T>
inline std::string print_vec(const std::vector<T> &list,
                             const std::string &sep = ", ") {
  std::string msg = "[";
  for (size_t i = 0; i < list.size(); ++i)
    msg +=
        std::format("{0} {1}", list[i], ((i == list.size() - 1) ? "]\n" : sep));
  return msg;
}

/* Formats `types::block` to pretty-print friendly representation
 */
inline std::string print_block(const block &b, std::size_t size) {
  std::string msg;
  for (std::size_t i = 0; i < size; ++i)
    msg += std::bitset<8>(b[i]).to_string() + " ";
  return msg;
}

/* Resolves params (TIMESTAMP, BLOCK_SIZE, NUM_BLOCKS) from `stdin` or returns
 * default values when not informed.
 *
 * Exits with error status if invalid input is passed in.
 */
inline program_args capture_args(int argc, const char **argv,
                                 bool verbose = false) {
  switch (argc) {
  case 2:
    if (verbose) {
      std::cout << "\033[33m[BLOCK_SIZE, NUM_BLOCKS] não informados; "
                   "usando parâmetros padrão: \033[0m"
                << std::endl;
      print_vec<int>({DEFAULT_BLOCK_SIZE, DEFAULT_NUM_BLOCKS});
    }

    return program_args(argv[1], DEFAULT_BLOCK_SIZE, DEFAULT_NUM_BLOCKS);
    break;
  case 3:
    if (verbose) {
      std::cout << "\033[33mUsando parâmetros [BLOCK_SIZE] "
                   "informados: \033[0m"
                << std::endl;
      print_vec(std::vector<std::string>(argv + 2, argv + argc));

      std::cout << "\033[33m[NUM_BLOCKS] não informados; usando "
                   "parâmetros padrão: \033[0m"
                << std::endl;
      print_vec<int>({DEFAULT_NUM_BLOCKS});
    }

    return program_args(argv[1], std::stoi(argv[2]), DEFAULT_NUM_BLOCKS);
    break;
  case 4:
    if (verbose) {
      std::cout << "\033[33mUsando parâmetros [BLOCK_SIZE, NUM_BLOCKS] "
                   "informados: \033[0m"
                << std::endl;
      print_vec(std::vector<std::string>(argv + 2, argv + argc));
    }

    return program_args(argv[1], std::stoi(argv[2]), std::stoi(argv[3]));
    break;
  default:
    if (verbose) {
      std::cerr << "\033[31mEntrada inválida: \033[0m" << std::endl;
      print_vec(std::vector<std::string>(argv + 2, argv + argc));
      std::cout << "\033[31mDeve ser " << argv[0] << " <BLOCK_SIZE>\033[0m ou"
                << std::endl;
      std::cout << "\033[31mDeve ser " << argv[0]
                << " <BLOCK_SIZE, NUM_BLOCKS>\033[0m" << std::endl;
    }

    std::exit(EXIT_FAILURE);
  }
}

/* Validates parameters interpreted from `stdin` according to application logic.
 *
 * Exits with error status if invalid input is passed in.
 */
inline void validate_args(program_args &args, int world_size,
                          bool verbose = false) {
  auto fail = [&](const std::string &msg) {
    if (verbose) {
      std::cerr << msg << std::endl;
    }
    std::exit(EXIT_FAILURE);
  };

  int block_size = std::get<1>(args);
  int num_blocks = std::get<2>(args);

  if (num_blocks <= 0)
    fail("Número de blocos de memória alocados deve ser maior do que 0");

  if (block_size <= 0)
    fail("Tamanho de bloco de memória deve ser maior do que 0");

  if (num_blocks < world_size)
    fail("Número de blocos de memória alocados deve ser igual ou maior ao "
         "número de processos instanciados");

  if (num_blocks > MAX_NUM_BLOCKS)
    fail(std::format(
        "Limite máximo de número de blocos de memória alocados é {0}",
        MAX_NUM_BLOCKS));

  if (block_size > MAX_BLOCK_SIZE)
    fail(std::format(
        "Limite máximo do tamanho dos blocos de memória alocados é {0}",
        MAX_BLOCK_SIZE));
}

/* Determines if process instance should produce verbose output.
 *
 * To maintain clarity and readability in the output, only process rank 0 should
 * display the major lifecycle admonitions.
 */
inline bool is_verbose(int world_rank) {
  return world_rank == MASTER_INSTANCE_ID;
}

/* Resolves the `memory_map` representing the memory block distribution between
 * the program instances
 */
inline memory_map resolve_maintainers(int world_size, int num_blocks) {
  std::vector<std::vector<int>> assignment(world_size);
  for (int i = 0; i < num_blocks; ++i) {
    assignment[i % world_size].push_back(i);
  }

  return assignment;
}

inline int resolve_maintainer(int world_size, int key) {
  return key % world_size;
}

/* Adds a process-rank identifier tag to the beggining of the message
 */
inline std::string identify_log_string(std::string input, int world_rank) {
  return std::format("(@proc {0}) ", world_rank) + input;
}

/* Calculates the total size (in bytes) of a WRITE message buffer, considering
 * the following buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ block data {BLOCK_SIZE bytes} ]`
 */
int get_total_write_message_size() {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int block_size = registry->get(GlobalRegistryIndex::BlockSize);
  return sizeof(int) + block_size;
}

/* Encodes a WRITE message from `key`, `value` to the buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ block data {BLOCK_SIZE bytes} ]`
 */
inline std::unique_ptr<uint8_t[]> encode_write_message(int key, block value) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int block_size = registry->get(GlobalRegistryIndex::BlockSize);

  int total_size = get_total_write_message_size();
  std::unique_ptr<uint8_t[]> message_buffer =
      std::make_unique<uint8_t[]>(total_size);

  std::memcpy(message_buffer.get(), &key, sizeof(int));
  std::memcpy(message_buffer.get() + sizeof(int), value.get(), block_size);

  return message_buffer;
}

/* Decodes a WRITE message to `WriteMessageBuffer`, from  the buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ block data {BLOCK_SIZE bytes} ]`
 */
inline std::unique_ptr<WriteMessageBuffer>
decode_write_message(std::unique_ptr<uint8_t[]> message_buffer) {
  std::shared_ptr<GlobalRegistry> registry = GlobalRegistry::get_instance();
  int block_size = registry->get(GlobalRegistryIndex::BlockSize);

  int key;
  std::unique_ptr<uint8_t[]> value = std::make_unique<uint8_t[]>(block_size);

  std::memcpy(&key, message_buffer.get(), sizeof(int));
  std::memcpy(value.get(), message_buffer.get() + sizeof(int), block_size);

  return std::make_unique<WriteMessageBuffer>(
      WriteMessageBuffer(key, std::move(value)));
}

#endif

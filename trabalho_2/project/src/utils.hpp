#ifndef __UTILS_H__
#define __UTILS_H__

#include "constants.hpp"
#include "logger.hpp"
#include "store.hpp"
#include "types.hpp"
#include <bitset>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

/**
 * Formats `std::vector<T>` to pretty-print friendly representation
 */
template <typename T>
inline std::string print_vec(const std::vector<T> &list,
                             const std::string &sep = ", ") {
  std::string msg = "[";
  for (size_t i = 0; i < list.size(); ++i)
    msg +=
        std::format("{0}{1}", list[i], ((i == list.size() - 1) ? "]\n" : sep));
  return msg;
}

/**
 * Formats `types::block` (of length BLOCK_SIZE) to pretty-print friendly
 * representation
 */
inline std::string print_block(const block &b) {
  std::string msg;
  for (int i = 0; i < registry_get(GlobalRegistryIndex::BlockSize); ++i) {
    msg += std::bitset<8>(b[i]).to_string() + " ";
  }
  return msg;
}

/**
 * Formats byte arrays of variable size to pretty-print friendly representation
 */
inline std::string print_block(std::shared_ptr<uint8_t[]> b, int size) {
  std::string msg;
  for (int i = 0; i < size; ++i) {
    msg += std::bitset<8>(b[i]).to_string() + " ";
  }
  return msg;
}

/**
 * Resolves params (TIMESTAMP, BLOCK_SIZE, NUM_BLOCKS) from `stdin` or returns
 * default values when not informed.
 *
 * Exits with error status if invalid input is passed in.
 */
inline program_args capture_args(int argc, const char **argv,
                                 bool verbose = false) {
  switch (argc) {
  case 3:
    if (verbose) {
      std::cout << "\033[33m[BLOCK_SIZE, NUM_BLOCKS] não informados; "
                   "usando parâmetros padrão: \033[0m"
                << std::endl
                << print_vec<int>({DEFAULT_BLOCK_SIZE, DEFAULT_NUM_BLOCKS})
                << std::endl;
    }

    return program_args(std::stoi(argv[1]), argv[2], DEFAULT_BLOCK_SIZE,
                        DEFAULT_NUM_BLOCKS);
    break;
  case 4:
    if (verbose) {
      std::cout << "\033[33mUsando parâmetros [BLOCK_SIZE] "
                   "informados: \033[0m"
                << std::endl
                << print_vec(std::vector<std::string>(argv + 3, argv + argc))
                << std::endl;

      std::cout << "\033[33m[NUM_BLOCKS] não informados; usando "
                   "parâmetros padrão: \033[0m"
                << std::endl
                << print_vec<int>({DEFAULT_NUM_BLOCKS}) << std::endl;
    }

    return program_args(std::stoi(argv[1]), argv[2], std::stoi(argv[3]),
                        DEFAULT_NUM_BLOCKS);
    break;
  case 5:
    if (verbose) {
      std::cout << "\033[33mUsando parâmetros [BLOCK_SIZE, NUM_BLOCKS] "
                   "informados: \033[0m"
                << std::endl
                << print_vec(std::vector<std::string>(argv + 3, argv + argc))
                << std::endl;
    }

    return program_args(std::stoi(argv[1]), argv[2], std::stoi(argv[3]),
                        std::stoi(argv[4]));
    break;
  default:
    if (verbose) {
      std::cerr << "\033[31mEntrada inválida: \033[0m" << std::endl
                << print_vec(std::vector<std::string>(argv + 3, argv + argc))
                << std::endl;
      std::cout << "\033[31mDeve ser " << argv[0] << " <BLOCK_SIZE>\033[0m ou"
                << std::endl;
      std::cout << "\033[31mDeve ser " << argv[0]
                << " <BLOCK_SIZE, NUM_BLOCKS>\033[0m" << std::endl;
    }

    std::exit(EXIT_FAILURE);
  }
}

/**
 * Compute number of "worker" processes, that is, instances that act as memory
 * block maintainers and perform read/write operations
 */
inline int get_num_worker_procs(int world_size) { return world_size - 1; }

/**
 * Get the world rank of the broadcaster instance
 */
inline int get_broadcaster_proc_rank(int world_size) { return world_size - 1; }

/**
 * Validates parameters interpreted from `stdin` according to application
 * logic.
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

  int log_level = std::get<0>(args);
  int block_size = std::get<2>(args);
  int num_blocks = std::get<3>(args);
  std::set<int> levels = {LOG_LEVEL_SPARSE, LOG_LEVEL_REGULAR,
                          LOG_LEVEL_VERBOSE};

  if (!levels.contains(log_level))
    fail(std::format(
        "`LOG_LEVEL` deve ser um de [0, 1, 2], mas foi informado o valor {0}",
        log_level));

  if (num_blocks <= 0)
    fail("Número de blocos de memória alocados deve ser maior do que 0");

  if (block_size <= 0)
    fail("Tamanho de bloco de memória deve ser maior do que 0");

  if (num_blocks < get_num_worker_procs(world_size))
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

/**
 * Determines if process instance should produce verbose output.
 *
 * To maintain clarity and readability in the output, only process rank 0
 * should display the major lifecycle admonitions.
 */
inline bool is_verbose(int world_rank) {
  return world_rank == MASTER_INSTANCE_ID;
}

/**
 * Resolves the `memory_map` representing the memory block distribution
 * between the program instances
 */
inline memory_map resolve_maintainers() {
  int world_size =
      get_num_worker_procs(registry_get(GlobalRegistryIndex::WorldSize));
  int num_blocks = registry_get(GlobalRegistryIndex::NumBlocks);

  std::vector<std::vector<int>> assignment(world_size);
  for (int i = 0; i < num_blocks; ++i) {
    assignment[i % world_size].push_back(i);
  }

  return assignment;
}

/**
 * Resolves the maintainer process' ID based on the desired block
 */
inline int resolve_maintainer(int key) {
  return key %
         get_num_worker_procs(registry_get(GlobalRegistryIndex::WorldSize));
}

/**
 * Calculates the total size (in bytes) of a WRITE message buffer, considering
 * the following buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ block data {BLOCK_SIZE bytes} ]`
 */
inline int get_total_write_message_buffer_size() {
  return sizeof(int) + registry_get(GlobalRegistryIndex::BlockSize);
}

// clang-format off

/**
 * Calculates the total size (in bytes) of a NOTIFICATION message buffer,
 * considering the following buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ long timestamp {sizeof(long) bytes} ]`
 */
inline int get_total_notification_message_buffer_size() {
  // clang-format on
  return sizeof(int) + sizeof(long);
}

/**
 * Encodes a WRITE message from `WriteMessageBuffer` to the buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ block data {BLOCK_SIZE bytes} ]`
 */
inline std::shared_ptr<uint8_t[]>
encode_write_message(WriteMessageBuffer &message) {
  int total_size = get_total_write_message_buffer_size();
  int key = message.key;
  block value = message.data;

  std::shared_ptr<uint8_t[]> message_buffer =
      std::make_shared<uint8_t[]>(total_size);

  std::memcpy(message_buffer.get(), &key, sizeof(int));
  std::memcpy(message_buffer.get() + sizeof(int), value.get(),
              registry_get(GlobalRegistryIndex::BlockSize));

  thread_safe_log_with_id(std::format("Encoding write message from of key: "
                                      "{0}, value: {1} as bytearray buffer {2}",
                                      message.key, print_block(message.data),
                                      print_block(message_buffer, total_size)));

  return message_buffer;
}

/**
 * Decodes a WRITE message to `WriteMessageBuffer`, from  the buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ block data {BLOCK_SIZE bytes} ]`
 */
inline WriteMessageBuffer
decode_write_message(std::shared_ptr<uint8_t[]> message_buffer) {
  int block_size = registry_get(GlobalRegistryIndex::BlockSize);
  int total_size = get_total_write_message_buffer_size();

  thread_safe_log_with_id(
      std::format("Decoding write message from bytearray buffer: {0}",
                  print_block(message_buffer, total_size)));

  int key;
  block value = std::make_shared<uint8_t[]>(block_size);

  thread_safe_log_with_id("Allocated memory for buffer");

  std::memcpy(&key, message_buffer.get(), sizeof(int));
  std::memcpy(value.get(), message_buffer.get() + sizeof(int), block_size);

  thread_safe_log_with_id("Copied buffer to memory");

  WriteMessageBuffer result(key, value);

  thread_safe_log_with_id(std::format(
      "Constructed object: key {0}, value {1} from raw message buffer {2}",
      result.key, print_block(result.data),
      print_block(message_buffer, total_size)));

  return result;
}

// clang-format off

/**
 * Encodes a NOTIFICATION message from `NotificationMessageBuffer` to the
 * buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ long timestamp {sizeof(long) bytes} ]`
 */
inline std::shared_ptr<uint8_t[]>
encode_notification_message(NotificationMessageBuffer &message) {
  // clang-format on
  int total_size = get_total_notification_message_buffer_size();
  int key = message.key;
  long timestamp = message.timestamp;

  std::shared_ptr<uint8_t[]> message_buffer =
      std::make_shared<uint8_t[]>(total_size);

  std::memcpy(message_buffer.get(), &key, sizeof(int));
  std::memcpy(message_buffer.get() + sizeof(int), &timestamp, sizeof(long));

  thread_safe_log_with_id(std::format(
      "Encoding notification message from of key: "
      "{0}, timestamp: {1} as bytearray buffer {2}",
      message.key, timestamp, print_block(message_buffer, total_size)));

  return message_buffer;
}

// clang-format off

/**
 * Decodes a NOTIFICATION message to `NotificationMessageBuffer`, from  the buffer layout:
 *
 * `[ int target_index {sizeof(int) bytes} ][ long timestamp {sizeof(long) bytes} ]`
 */
inline NotificationMessageBuffer
decode_notificaton_message(std::shared_ptr<uint8_t[]> message_buffer) {
  // clang-format on
  int total_size = get_total_notification_message_buffer_size();

  thread_safe_log_with_id(
      std::format("Decoding notification message from bytearray buffer: {0}",
                  print_block(message_buffer, total_size)));

  int key;
  long timestamp;

  thread_safe_log_with_id("Allocated memory for buffer");

  std::memcpy(&key, message_buffer.get(), sizeof(int));
  std::memcpy(&timestamp, message_buffer.get() + sizeof(int), sizeof(long));

  thread_safe_log_with_id("Copied buffer to memory");

  NotificationMessageBuffer result(key, timestamp);

  thread_safe_log_with_id(std::format(
      "Constructed object: key {0}, timestamp {1} from "
      "raw message buffer {2}",
      result.key, timestamp, print_block(message_buffer, total_size)));

  return result;
}

inline block get_random_block() {
  int block_size = registry_get(GlobalRegistryIndex::BlockSize);
  block buffer = std::make_shared<uint8_t[]>(block_size);

  for (int i = 0; i < block_size; i++)
    buffer[i] = rand() % 256;

  return buffer;
}

inline block get_random_block(int size) {
  block buffer = std::make_shared<uint8_t[]>(size);

  for (int i = 0; i < size; i++)
    buffer[i] = rand() % 256;

  return buffer;
}

#endif

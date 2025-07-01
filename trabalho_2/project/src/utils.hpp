#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.hpp"
#include <bitset>
#include <format>
#include <iostream>
#include <vector>

#define DEFAULT_BLOCK_SIZE 8
#define DEFAULT_NUM_BLOCKS 4
#define MAX_BLOCK_SIZE 32
#define MAX_NUM_BLOCKS 32
#define MASTER_INSTANCE_ID 0

#define MESSAGE_TAG_REQUEST 0

/* Pretty-prints `std::block` representation to `stdout`.
 */
template <typename T>
inline void print_vec(const std::vector<T> &list,
                      const std::string &sep = ", ") {
  std::cout << "[";
  for (size_t i = 0; i < list.size(); ++i)
    std::cout << list[i] << ((i == list.size() - 1) ? "]\n" : sep);
}

/* Pretty-prints `types::block` representation to `stdout`.
 */
inline void print_block(const block &b, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i)
    std::cout << std::bitset<8>(b[i]) << " ";
  std::cout << std::endl;
}

/* Resolves params (BLOCK_SIZE, NUM_BLOCKS) from `stdin` or returns default
 * values when not informed.
 *
 * Exits with error status if invalid input is passed in.
 */
inline program_args capture_args(int argc, const char **argv,
                                 bool verbose = false) {
  switch (argc) {
  case 1:
    if (verbose) {
      std::cout << "\033[33m[BLOCK_SIZE, NUM_BLOCKS] não informados; "
                   "usando parâmetros padrão: \033[0m"
                << std::endl;
      print_vec<int>({DEFAULT_BLOCK_SIZE, DEFAULT_NUM_BLOCKS});
    }

    return program_args(DEFAULT_BLOCK_SIZE, DEFAULT_NUM_BLOCKS);
    break;
  case 2:
    if (verbose) {
      std::cout << "\033[33mUsando parâmetros [BLOCK_SIZE] "
                   "informados: \033[0m"
                << std::endl;
      print_vec(std::vector<std::string>(argv + 1, argv + argc));

      std::cout << "\033[33m[NUM_BLOCKS] não informados; usando "
                   "parâmetros padrão: \033[0m"
                << std::endl;
      print_vec<int>({DEFAULT_NUM_BLOCKS});
    }

    return program_args(std::stoi(argv[1]), DEFAULT_NUM_BLOCKS);
    break;
  case 3:
    if (verbose) {
      std::cout << "\033[33mUsando parâmetros [BLOCK_SIZE, NUM_BLOCKS] "
                   "informados: \033[0m"
                << std::endl;
      print_vec(std::vector<std::string>(argv + 1, argv + argc));
    }

    return program_args(std::stoi(argv[1]), std::stoi(argv[2]));
    break;
  default:
    if (verbose) {
      std::cerr << "\033[31mEntrada inválida: \033[0m" << std::endl;
      print_vec(std::vector<std::string>(argv + 1, argv + argc));
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

  int block_size = std::get<0>(args);
  int num_blocks = std::get<1>(args);

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

#endif

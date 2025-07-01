#ifndef __LIB_H__
#define __LIB_H__

#include "types.hpp"
#include "utils.hpp"
#include <iostream>
#include <map>

/* Wrapper class for the process-local memory-blocks - that is - the memory
 * blocks that are maintained by the local process.
 */
class LocalRepository {
public:
  LocalRepository(memory_map mem_map, int block_size, int world_rank);

private:
  memory_map mem_map;
  std::map<int, block> blocks;
};

inline LocalRepository::LocalRepository(memory_map mem_map, int block_size,
                                        int world_rank) {
  this->mem_map = mem_map;
  this->blocks = std::map<int, block>();

  for (int i : mem_map.at(world_rank)) {
    block b = std::make_unique<std::uint8_t[]>(block_size);
    std::fill_n(b.get(), block_size, 0);
    blocks.emplace(i, std::move(b));
  }

  if (world_rank == 0) {
    for (auto i : mem_map.at(world_rank)) {
      std::cout << "{" << std::endl << i << ": ";
      print_block(std::move(blocks.at(i)), block_size);
      std::cout << "}" << std::endl;
    }
  }
}

#endif

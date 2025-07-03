#include "store.hpp"
#include <exception>
#include <stdexcept>

std::shared_ptr<GlobalRegistry> GlobalRegistry::instance{nullptr};

GlobalRegistry::GlobalRegistry(int world_rank, int world_size, int num_blocks,
                               int block_size, int timestamp)
    : data() {
  data.emplace(GlobalRegistryIndex::WorldRank, world_rank);
  data.emplace(GlobalRegistryIndex::WorldSize, world_size);
  data.emplace(GlobalRegistryIndex::NumBlocks, num_blocks);
  data.emplace(GlobalRegistryIndex::BlockSize, block_size);
  data.emplace(GlobalRegistryIndex::Timestamp, timestamp);
}

/* Read entry from the registry
 */
int GlobalRegistry::get(GlobalRegistryIndex key) {
  try {
    return data.at(key);
  } catch (const std::exception &e) {
    throw std::runtime_error("bad index");
  }
}

/* Creates and returns the global registry instance
 */
std::shared_ptr<GlobalRegistry>
GlobalRegistry::get_instance(int world_rank, int world_size, int num_blocks,
                             int block_size, int timestamp) {
  if (instance.get() == nullptr) {
    instance = std::shared_ptr<GlobalRegistry>(new GlobalRegistry(
        world_rank, world_size, num_blocks, block_size, timestamp));
  }
  return instance;
};

/* Provides access to the global registry instance
 */
std::shared_ptr<GlobalRegistry> GlobalRegistry::get_instance() {
  if (instance.get() == nullptr) {
    throw std::runtime_error("global registry not initialized");
  }
  return instance;
};

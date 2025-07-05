#ifndef __STORE_H__
#define __STORE_H__

#include <map>
#include <memory>

enum class GlobalRegistryIndex {
  WorldSize,
  WorldRank,
  NumBlocks,
  BlockSize,
  Timestamp
};

/* Provides easy (read-only) static access to an instance-scoped immutable set
 * of attributes
 */
class GlobalRegistry {
protected:
  GlobalRegistry(int world_rank, int world_size, int num_blocks, int block_size,
                 int timestamp);

public:
  /* Read entry from the registry
   */
  int get(GlobalRegistryIndex key);

  /* Creates and returns the global registry instance
   */
  static std::shared_ptr<GlobalRegistry>
  get_instance(int world_rank, int world_size, int num_blocks, int block_size,
               int timestamp);

  /* Provides access to the global registry instance
   */
  static std::shared_ptr<GlobalRegistry> get_instance();

private:
  static std::shared_ptr<GlobalRegistry> instance;
  std::map<GlobalRegistryIndex, int> data;
};

#endif

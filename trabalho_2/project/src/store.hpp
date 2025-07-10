#ifndef __STORE_H__
#define __STORE_H__

#include <map>
#include <memory>

enum class GlobalRegistryIndex {
  WorldSize,
  WorldRank,
  NumBlocks,
  BlockSize,
  Timestamp,
  LogLevel,
};

/* Provides easy (read-only) static access to an instance-scoped immutable set
 * of attributes
 */
class GlobalRegistry {
protected:
  GlobalRegistry(int world_rank, int world_size, int num_blocks, int block_size,
                 int timestamp, int log_level);

public:
  /* Read entry from the registry
   */
  int get(GlobalRegistryIndex key);

  /* Creates and returns the global registry instance
   */
  static std::shared_ptr<GlobalRegistry>
  get_instance(int world_rank, int world_size, int num_blocks, int block_size,
               int timestamp, int log_level);

  /* Provides access to the global registry instance
   */
  static std::shared_ptr<GlobalRegistry> get_instance();

private:
  static std::shared_ptr<GlobalRegistry> instance;
  std::map<GlobalRegistryIndex, int> data;
};

/* Wrapper around static method call to `GlobalRegistry::get`
 */
inline int registry_get(GlobalRegistryIndex key) {
  return GlobalRegistry::get_instance()->get(key);
}

#endif

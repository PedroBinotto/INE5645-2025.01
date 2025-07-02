#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

/* Represents the input model for the program as a `std::tuple<int, int>`,
 * wherein:
 *
 * `std::get<0>(program_args)` returns `BLOCK_SIZE`
 * `std::get<1>(program_args)` returns `NUM_BLOCKS`
 */
typedef std::tuple<int, int> program_args;

/* "Block" datatype representation; each block is a bytearray size `BLOCK_SIZE`
 */
using block = std::unique_ptr<std::uint8_t[]>;

/* Represents the distributed memory allocation between the multiple instances
 * of the program, wherein:
 *
 * - indexes are integer values representing the `world_rank` of each instance;
 * - values (`std::vector<int>`) are lists containing the indexes of which
 * memory blocks are maintained by that instance
 */
typedef std::vector<std::vector<int>> memory_map;

#endif

#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

/* Represents the input model for the program as a `std::tuple<int, int>`,
 * wherein:
 *
 * `std::get<0>(program_args)` returns `LOG_LEVEL`
 * `std::get<1>(program_args)` returns `TIMESTAMP`
 * `std::get<2>(program_args)` returns `BLOCK_SIZE`
 * `std::get<3>(program_args)` returns `NUM_BLOCKS`
 */
typedef std::tuple<int, std::string, int, int> program_args;

/* Represents the server/listener thread model as a tuple, wherein:
 *
 * `std::get<0>(server_threads)` returns `read_listener`
 * `std::get<1>(server_threads)` returns `write_listener`
 * `std::get<2>(server_threads)` returns `notification_listener`
 */
typedef std::tuple<std::thread, std::thread, std::thread> server_threads;

/* "Block" datatype representation; each block is a bytearray size `BLOCK_SIZE`
 */
using block = std::shared_ptr<std::uint8_t[]>;

/* Represents the distributed memory allocation between the multiple instances
 * of the program, wherein:
 *
 * - indexes are integer values representing the `world_rank` of each instance;
 * - values (`std::vector<int>`) are lists containing the indexes of which
 * memory blocks are maintained by that instance
 */
typedef std::vector<std::vector<int>> memory_map;

/* `stuct` representation of the message buffer for WRITE messages to be sent
 * over MPI
 */
struct WriteMessageBuffer {
  int key;
  block data;
};

/* `stuct` representation of the message buffer for NOTIFICATION messages to be
 * sent over MPI
 */
struct NotificationMessageBuffer {
  int key;
  int64_t timestamp;
};

#endif

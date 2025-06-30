#ifndef __TYPES_H__
#define __TYPES_H__

#include <tuple>

/* Represents the input model for the program as a `std::tuple<int, int>`,
 * wherein:
 *
 * `std::get<0>(program_args)` returns `BLOCK_SIZE`
 * `std::get<1>(program_args)` returns `NUM_BLOCKS`
 */
typedef std::tuple<int, int> program_args;

#endif

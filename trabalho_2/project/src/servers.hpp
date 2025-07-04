#ifndef __SERVERS_H__
#define __SERVERS_H__

#include "lib.hpp"
#include "types.hpp"
#include <mpi.h>
#include <unistd.h>

/* Listener/subscriber loop that will handle incoming READ operations
 * identified by `MESSAGE_TAG_BLOCK_READ_REQUEST`
 */
void read_listener(memory_map mem_map, UnifiedRepositoryFacade &repo);

/* Listener/subscriber loop that will handle incoming WRITE operations
 * identified by `MESSAGE_TAG_BLOCK_WRITE_REQUEST`
 */
void write_listener(memory_map mem_map, UnifiedRepositoryFacade &repo);

/* Listener/subscriber loop that will handle incoming notifications
 * identified by `MESSAGE_TAG_BLOCK_UPDATE_NOTIFICATION`
 */
void notification_listener(memory_map mem_map, UnifiedRepositoryFacade &repo);

#endif

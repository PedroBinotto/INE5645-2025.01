#ifndef __TYPES_H__
#define __TYPES_H__

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef std::tuple<int, int, int, int, int> program_args;

struct order {
  int id;
  int client_id;
  int preparation_time_millis;
  bool ready;

  order(int id, int client_id, int preparation_time_millis)
      : id(id), client_id(client_id), preparation_time_millis(preparation_time_millis) {}
};

typedef std::unordered_map<int, std::pair<std::string, std::vector<order>>> logbook;

struct stats {
  int time_open_millis;
  int total_orders_accepted;
  int total_orders_denied;
  logbook &orders_completed_by_cook;

  stats(int time_open_millis, int total_orders_accepted, int total_orders_denied, logbook orders_completed_by_cook)
      : time_open_millis(time_open_millis), total_orders_accepted(total_orders_accepted),
        total_orders_denied(total_orders_denied), orders_completed_by_cook(orders_completed_by_cook) {}
};

#endif

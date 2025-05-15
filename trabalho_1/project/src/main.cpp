#include "artifacts.hpp"
#include "utils.hpp"
#include <atomic>
#include <cstdlib>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct order {
  int id;
  int client_id;
  int preparation_time_millis;
  bool ready;

  order(int id, int client_id, int preparation_time_millis)
      : id(id), client_id(client_id), preparation_time_millis(preparation_time_millis) {}
};

bool is_open = true;
std::mutex cout_lock;
counting_semaphore ovens{2};
counting_semaphore stoves{4};
std::atomic<int> total_orders{0};
std::atomic<int> next_order_id{0};
std::vector<std::thread> cook_threads;
std::vector<std::thread> client_threads;
std::mt19937 rng{std::random_device{}()};
synchronizing_queue<order> orders_received;
synchronizing_queue<order> orders_completed;

/* Function to be passed to cook threads
 */
void cook(int id, std::string name, int time_offset_millis, barrier &kitchen_barrier) {
  std::function<std::string(std::string)> format_message = [id, name](std::string message) {
    return "Cozinheiro " + name + " (id: " + std::to_string(id) + ") " + message;
  };

  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset_millis));
  thread_safe_print(format_message("chegou para trabalhar"), cout_lock);

  kitchen_barrier.wait();

  thread_safe_print(format_message("começou a trabalhar"), cout_lock);

  while (is_open || !orders_received.empty()) {
    std::optional<order> maybe_order = orders_received.pop();

    if (!maybe_order.has_value())
      break;

    order current = maybe_order.value();

    thread_safe_print(format_message("começou a preparar pedido " + std::to_string(current.id) + " para cliente " +
                                     std::to_string(current.client_id)),
                      cout_lock);

    bool needs_stove = (rng() % 2 == 0);
    bool needs_oven = (rng() % 2 == 0);
  }
}

/* Function to be passed to client threads
 */
void client(int id, int time_offset_millis) {
  std::function<std::string(std::string)> format_message = [id](std::string message) {
    return "Cliente " + std::to_string(id) + " " + message;
  };

  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset_millis));
  thread_safe_print(format_message("chegou no restaurante"), cout_lock);

  order client_order(next_order_id++, id, get_random_delay_millis());

  thread_safe_print(format_message("realizou pedido de id " + std::to_string(client_order.id)), cout_lock);

  orders_received.push(client_order);
  total_orders++;

  thread_safe_print(format_message("aguardando pedido " + std::to_string(client_order.id)), cout_lock);
}

void run_simulation(int cooks, int clients) {
  barrier kitchen_barrier(cooks + 1);

  thread_safe_print("Restaurante abrirá em breve", cout_lock);

  for (int i = 1; i <= cooks; i++) {
    cook_threads.emplace_back(cook, i, generate_cook_name(), get_random_delay_millis(), std::ref(kitchen_barrier));
  }

  kitchen_barrier.wait();
  thread_safe_print("Cozinha está funcionando!", cout_lock);

  for (int i = 1; i <= clients; i++) {
    client_threads.emplace_back(client, i, get_random_delay_millis());
  }

  for (std::thread &t : client_threads) {
    t.join();
  }
}

int main(int argc, const char **argv) {
  std::pair<int, int> params = capture_args(argc, argv);

  print_greeting();
  thread_safe_print("\033[32mIniciando simulação\033[0m", cout_lock);

  run_simulation(params.first, params.second);

  return 0;
}

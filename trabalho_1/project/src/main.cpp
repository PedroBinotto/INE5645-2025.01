#include "artifacts.hpp"
#include "utils.hpp"
#include <atomic>
#include <cstdlib>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct order {
  int id;
  int customer_id;
  int preparation_time_millis;
  bool ready;

  order(int id, int customer_id, int preparation_time_millis)
      : id(id), customer_id(customer_id), preparation_time_millis(preparation_time_millis) {}
};

bool is_open = true;
std::mutex cout_lock;
std::atomic<int> total_orders{0};
std::atomic<int> next_order_id{0};
synchronizing_queue<order> orders_received;
synchronizing_queue<order> orders_completed;

void cook(int id, std::string name, int time_offset_millis, barrier &kitchen_barrier) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset_millis));
  thread_safe_print("Cozinheiro " + name + " (id: " + std::to_string(id) + ") chegou para trabalhar", cout_lock);
  kitchen_barrier.wait();
}

void client(int id, int time_offset_millis) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset_millis));
  thread_safe_print("Cliente " + std::to_string(id) + " chegou no restaurante", cout_lock);

  order client_order(next_order_id++, id, get_random_delay_millis());

  thread_safe_print("Cliente " + std::to_string(id) + " realizou pedido de id " + std::to_string(client_order.id),
                    cout_lock);

  orders_received.push(client_order);
  total_orders++;
}

void run_simulation(int cooks, int clients) {
  barrier kitchen_barrier(cooks + 1);
  std::vector<std::thread> cook_threads;
  std::vector<std::thread> client_threads;
  std::mt19937 rng{std::random_device{}()};

  thread_safe_print("Restaurante abrirá em breve", cout_lock);

  for (int i = 1; i <= cooks; ++i) {
    cook_threads.emplace_back(cook, i, generate_cook_name(), get_random_delay_millis(), std::ref(kitchen_barrier));
  }

  kitchen_barrier.wait();
  thread_safe_print("Cozinha está funcionando!", cout_lock);

  for (int i = 1; i <= clients; ++i) {
    client_threads.emplace_back(client, i, get_random_delay_millis());
  }

  for (std::thread &t : client_threads) {
    t.join();
  }
}

int main(int argc, const char **argv) {
  std::pair<int, int> params = capture_args(argc, argv);

  print_greeting();
  thread_safe_print("Iniciando simulação", cout_lock);

  run_simulation(params.first, params.second);

  return 0;
}

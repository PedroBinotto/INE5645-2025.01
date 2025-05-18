#include "artifacts.hpp"
#include "utils.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#define RESTAURANT_CLOSING_TIME_MILLIS 15000 // 15 seconds

struct order {
  int id;
  int client_id;
  int preparation_time_millis;
  bool ready;

  order(int id, int client_id, int preparation_time_millis)
      : id(id), client_id(client_id), preparation_time_millis(preparation_time_millis) {}
};

std::mutex cout_lock;
std::atomic<bool> is_open = true;
std::atomic<int> total_orders{0};
std::atomic<int> next_order_id{0};
synchronizing_queue<order> orders_in;
std::vector<std::thread> cook_threads;
std::vector<std::thread> client_threads;
std::mt19937 rng{std::random_device{}()};
observer_subject<int, order> subscriptions;

/* Function to be passed to timer thread. Closes the restaurant for new orders after `time_millis` milliseconds
 */
void timer(int time_millis) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time_millis));
  thread_safe_print("Restaurante fechando, sem mais pedidos", cout_lock);
  is_open = false;
}

/* Function to be passed to cook threads
 */
void cook(int id, std::string name, int time_offset_millis, barrier &kitchen_barrier, counting_semaphore &ovens,
          counting_semaphore &stoves) {
  std::function<std::string(std::string)> format_message = [id, name](std::string message) {
    return "Cozinheiro " + name + " (id: " + std::to_string(id) + ") " + message;
  };

  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset_millis));
  thread_safe_print(format_message("chegou para trabalhar"), cout_lock);

  kitchen_barrier.wait();

  thread_safe_print(format_message("começou a trabalhar"), cout_lock);

  while (is_open || !orders_in.empty()) {
    std::optional<order> maybe_order = orders_in.pop();

    if (!maybe_order.has_value())
      break;

    order current = maybe_order.value();

    thread_safe_print(format_message("começou a preparar pedido " + std::to_string(current.id) + " para cliente " +
                                     std::to_string(current.client_id)),
                      cout_lock);

    bool use_stove = (rng() % 2 == 0);
    bool use_oven = (rng() % 2 == 0);

    if (use_stove) {
      thread_safe_print(format_message("esperando para usar fogão"), cout_lock);
      stoves.acquire();
      thread_safe_print(format_message("usando fogão"), cout_lock);
    }
    if (use_oven) {
      thread_safe_print(format_message("esperando para usar forno"), cout_lock);
      ovens.acquire();
      thread_safe_print(format_message("usando forno"), cout_lock);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(current.preparation_time_millis));

    if (use_stove) {
      thread_safe_print(format_message("liberou fogão"), cout_lock);
      stoves.release();
    }
    if (use_oven) {
      thread_safe_print(format_message("liberou forno"), cout_lock);
      ovens.release();
    }

    current.ready = true;
    thread_safe_print(format_message("finalizou pedido " + std::to_string(current.id) + " do cliente " +
                                     std::to_string(current.client_id)),
                      cout_lock);

    subscriptions.notify(current.id, current);
  }

  thread_safe_print(format_message("encerrou o turno"), cout_lock);
}

/* Function to be passed to client threads
 */
void client(int id, int time_offset_millis) {
  std::function<std::string(std::string)> format_message = [id](std::string message) {
    return "Cliente " + std::to_string(id) + " " + message;
  };

  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset_millis));
  thread_safe_print(format_message("chegou no restaurante"), cout_lock);

  if (is_open) {
    std::mutex order_mutex;
    bool order_received = false;
    std::condition_variable order_cv;
    order client_order(next_order_id++, id, get_random_delay_millis());

    thread_safe_print(format_message("realizou pedido de id " + std::to_string(client_order.id)), cout_lock);

    subscriptions.subscribe(client_order.id, [&](const order &_) {
      {
        std::lock_guard<std::mutex> lock(order_mutex);
        order_received = true;
      }
      order_cv.notify_one();
    });

    orders_in.push(client_order);
    total_orders++;

    thread_safe_print(format_message("aguardando pedido " + std::to_string(client_order.id)), cout_lock);

    {
      std::unique_lock<std::mutex> lock(order_mutex);
      order_cv.wait(lock, [&] { return order_received; });
    }

    thread_safe_print(format_message("recebeu pedido " + std::to_string(client_order.id)), cout_lock);
  } else {
    thread_safe_print(format_message("desistiu de fazer um pedido pois restaurante está fechado"), cout_lock);
  }

  thread_safe_print(format_message("foi embora"), cout_lock);
}

void run_simulation(int n_cooks, int n_clients, int n_ovens, int n_stoves) {
  barrier kitchen_barrier(n_cooks + 1);
  counting_semaphore ovens{n_ovens};
  counting_semaphore stoves{n_stoves};

  thread_safe_print("Restaurante abrirá em breve", cout_lock);

  for (int i = 1; i <= n_cooks; i++) {
    cook_threads.emplace_back(cook, i, generate_cook_name(), get_random_delay_millis(), std::ref(kitchen_barrier),
                              std::ref(ovens), std::ref(stoves));
  }

  kitchen_barrier.wait();
  thread_safe_print("Cozinha está funcionando!", cout_lock);

  std::thread closing_timer(timer, RESTAURANT_CLOSING_TIME_MILLIS);

  // Time relief so that text remains readable before orders start piling up
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  for (int i = 1; i <= n_clients; i++) {
    client_threads.emplace_back(client, i, get_random_delay_millis(20));
  }

  for (std::thread &t : client_threads) {
    t.join();
  }

  closing_timer.join();
}

int main(int argc, const char **argv) {
  std::tuple<int, int, int, int> params = capture_args(argc, argv);

  print_greeting();

  std::apply(run_simulation, params);

  return 0;
}

#include "artifacts.hpp"
#include "utils.hpp"
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

std::mutex cout_lock;
bool open;

void cook(int id, std::string name, int time_offset, barrier &kitchen_barrier) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time_offset));
  thread_safe_print("Cozinheiro " + name + " (id: " + std::to_string(id) + ") chegou para trabalhar", cout_lock);
  kitchen_barrier.wait();
}

void client(int id) {}

void run_simulation(int cooks, int clients) {
  barrier kitchen_barrier(cooks + 1);
  std::vector<std::thread> cook_threads;
  std::mt19937 rng{std::random_device{}()};

  std::cout << "Restaurante abrirá em breve" << std::endl;

  for (int i = 1; i <= cooks; ++i) {
    int time = (system_time() % 9) * 1000;
    cook_threads.emplace_back(cook, i, generate_cook_name(), time, std::ref(kitchen_barrier));
  }

  kitchen_barrier.wait();

  open = true;
  std::cout << "Cozinha está funcionando!" << std::endl;
}

int main(int argc, const char **argv) {
  std::pair<int, int> params = capture_args(argc, argv);

  print_greeting();
  std::cout << "Iniciando simulação" << std::endl;

  run_simulation(params.first, params.second);

  return 0;
}

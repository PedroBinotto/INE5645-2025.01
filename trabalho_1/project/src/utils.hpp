#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.hpp"
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <ostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#define DEFAULT_NUM_COZINHEIROS 5
#define DEFAULT_NUM_CLIENTES 30
#define DEFAULT_NUM_FOGOES 2
#define DEFAULT_NUM_FORNOS 2
#define DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS 15

/* Pretty-prints vector representation to `stdout`.
 */
template <typename T> inline void print_vec(const std::vector<T> &list, const std::string &sep = ", ") {
  std::cout << "[";
  for (size_t i = 0; i < list.size(); ++i)
    std::cout << list[i] << ((i == list.size() - 1) ? "]\n" : sep);
}

/* Resolves system time (UNIX epoch) to milliseconds
 */
inline int system_time() { return std::chrono::system_clock::now().time_since_epoch().count() % 100000000; }

/* Resolves params (NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS, TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS) from
 * `stdin` or returns default values when not informed.
 *
 * Exits with error status if invalid input is passed in.
 */
inline program_args capture_args(int argc, const char **argv) {
  switch (argc) {
  case 1:
    std::cout << "\033[33m[NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS, "
                 "TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS] não informados; "
                 "usando parâmetros padrão: \033[0m"
              << std::endl;
    print_vec<int>({DEFAULT_NUM_COZINHEIROS, DEFAULT_NUM_CLIENTES, DEFAULT_NUM_FOGOES, DEFAULT_NUM_FORNOS,
                    DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS});

    return program_args(DEFAULT_NUM_COZINHEIROS, DEFAULT_NUM_CLIENTES, DEFAULT_NUM_FOGOES, DEFAULT_NUM_FORNOS,
                        DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS);
    break;
  case 3:
    std::cout << "\033[33mUsando parâmetros [NUM_COZINHEIROS, NUM_CLIENTES] informados: \033[0m" << std::endl;
    print_vec(std::vector<std::string>(argv + 1, argv + argc));

    std::cout << "\033[33m[NUM_FOGOES, NUM_FORNOS, TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS] não informados; usando "
                 "parâmetros padrão: \033[0m"
              << std::endl;
    print_vec<int>({DEFAULT_NUM_FOGOES, DEFAULT_NUM_FORNOS, DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS});

    return program_args(std::stoi(argv[1]), std::stoi(argv[2]), DEFAULT_NUM_FOGOES, DEFAULT_NUM_FORNOS,
                        DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS);
    break;
  case 5:
    std::cout << "\033[33mUsando parâmetros [NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS] informados: \033[0m"
              << std::endl;
    print_vec(std::vector<std::string>(argv + 1, argv + argc));

    std::cout << "\033[33m[TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS] não informado; usando parâmetro padrão: \033[0m"
              << std::endl;
    print_vec<int>({DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS});

    return program_args(std::stoi(argv[1]), std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]),
                        DEFAULT_TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS);
    break;
  case 6:
    std::cout << "\033[33mUsando parâmetros [NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS, "
                 "TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS] informados: \033[0m"
              << std::endl;
    print_vec(std::vector<std::string>(argv + 1, argv + argc));

    return program_args(std::stoi(argv[1]), std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]),
                        std::stoi(argv[5]));
    break;
  default:
    std::cerr << "\033[31mEntrada inválida: \033[0m" << std::endl;
    print_vec(std::vector<std::string>(argv + 1, argv + argc));
    std::cout << "\033[31mDeve ser " << argv[0] << " <NUM_COZINHEIROS, NUM_CLIENTES>\033[0m ou" << std::endl;
    std::cout << "\033[31mDeve ser " << argv[0] << " <NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS>\033[0m ou"
              << std::endl;
    std::cout
        << "\033[31mDeve ser " << argv[0]
        << " <NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS, TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS>\033[0m"
        << std::endl;

    exit(1);
  }
}

/* Generates a random name for the cook thread
 */
inline std::string
generate_cook_name(std::vector<std::string> firstnames = {"Juliana", "Bruna", "Mateus", "Rafael", "Pedro", "Larissa",
                                                          "Odorico", "Gabriel", "Eduardo", "Patrícia"},
                   std::vector<std::string> lastnames = {"Silva", "Santos", "Oliveira", "Souza", "Lemos", "Mendizabal",
                                                         "Barbosa", "Binotto", "Dias", "Teixeira"}) {
  std::mt19937 rng{std::random_device{}()};

  std::uniform_int_distribution<int> firstname_d(0, firstnames.size() - 1);
  std::uniform_int_distribution<int> lastname_d(0, lastnames.size() - 1);

  return firstnames[firstname_d(rng)] + " " + lastnames[lastname_d(rng)];
}

/* Print blank line
 */
inline void print_blank_line() { std::cout << std::endl; }

/* Print horizontal break
 */
inline void print_hline() {
  print_blank_line();
  std::cout << "*****************************************************************************" << std::endl;
  print_blank_line();
}

/* Print greeting message to the screen.
 */
inline void print_greeting() {
  std::vector<std::string> frenchy_firstnames{"Jean",     "Pierre",  "Jacques",  "Michel", "Sophie",
                                              "Isabelle", "Camille", "Nathalie", "Anne"};
  std::vector<std::string> frenchy_lastnames{"Renault", "Martin", "Laurent", "Moreau",  "Dubois",
                                             "Lefèvre", "Moreau", "Dupont",  "Fontaine"};

  std::string head_cook_name = generate_cook_name(frenchy_firstnames, frenchy_lastnames);

  print_hline();
  std::cout << "Bienvenue, monsieur ou madame, seja calorosamente bem-vindo ao" << std::endl;
  std::cout << "Chez L’Exagéré, o restaurante francês onde a elegância é obrigatória e o" << std::endl;
  std::cout << "cardápio, praticamente ilegível." << std::endl;
  print_blank_line();
  std::cout << "Aqui, cada prato é uma obra de arte — e, convenhamos, o preço também." << std::endl;
  print_blank_line();
  std::cout << "Nosso menu é elaborado diariamente — ou, como preferimos dizer," << std::endl;
  std::cout << "emocionalmente — pelo renomado chef " << head_cook_name << ", que acredita firmemente" << std::endl;
  std::cout << "que manteiga é um estado de espírito." << std::endl;
  print_hline();

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  std::cout << "\033[32mIniciando simulação\033[0m" << std::endl;
}

/* Print goodbye message with stats
 */
inline void print_goodbye(stats s) {
  print_blank_line();
  std::cout << "Estatísticas finais: " << std::endl;
  print_hline();

  std::cout << "Tempo de operação da cozinha (segundos): " << s.time_open_millis << std::endl;
  print_blank_line();
  std::cout << "Total de pedidos realizados por clientes: " << s.total_orders_accepted << std::endl;
  print_blank_line();
  std::cout << "Total de pedidos negados (tentativas após fechamento): " << s.total_orders_denied << std::endl;
  print_blank_line();
  std::cout << "Total de pedidos preparados por cada cozinheiro:" << std::endl;

  for (auto e : s.orders_completed_by_cook) {
    print_blank_line();
    std::cout << "***" << std::endl;
    print_blank_line();
    std::cout << "Cozinheiro " << e.second.first << " (id: " << e.first << "): " << std::endl;
    std::cout << "\t" << "Total de pedidos realizados: " << e.second.second.size() << std::endl;
    for (auto o : e.second.second) {
      print_blank_line();
      std::cout << "\t\tID Pedido: " << o.id << std::endl;
      std::cout << "\t\tTempo de preparo (segundos): " << o.preparation_time_millis / 1000 << std::endl;
    }
  }

  print_blank_line();
  std::cout << "\033[32mSimulação finalizada\033[0m" << std::endl;
  print_hline();
}

/* Thread-safe implementation of stdout print (with timestamp)
 */
inline void thread_safe_print(const std::string &s, std::mutex &m) {
  std::lock_guard<std::mutex> lock(m);

  std::cout << "[" << system_time() << "] " << s << std::endl;
}

/* Returns random delay time between 0 and `max_seconds` seconds (DEFAULT=9).
 */
inline int get_random_delay_millis(int max_seconds = 9) { return (system_time() % max_seconds) * 1000; }

#endif

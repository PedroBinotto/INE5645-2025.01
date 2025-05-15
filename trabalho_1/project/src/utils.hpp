#include <cstdlib>
#include <iostream>
#include <mutex>
#include <ostream>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#define DEFAULT_NUM_COZINHEIROS 3
#define DEFAULT_NUM_CLIENTES 3

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

/* Resolves params (NUM_COZINHEIROS, NUM_CLIENTES) from `stdin` or returns default values when not informed.
 *
 * Exits with error status if invalid input is passed in.
 */
inline std::pair<int, int> capture_args(int argc, const char **argv) {
  switch (argc) {
  case 1:
    std::cout << "[NUM_COZINHEIROS, NUM_CLIENTES] não informados; usando parâmetros padrão: " << std::endl;
    print_vec<int>({DEFAULT_NUM_COZINHEIROS, DEFAULT_NUM_CLIENTES});

    return std::pair<int, int>(DEFAULT_NUM_COZINHEIROS, DEFAULT_NUM_CLIENTES);
    break;
  case 3:
    std::cout << "Usando parâmetros [NUM_COZINHEIROS, NUM_CLIENTES] informados: " << std::endl;
    print_vec(std::vector<std::string>(argv + 1, argv + argc));

    return std::pair<int, int>(std::stoi(argv[1]), std::stoi(argv[2]));
    break;
  default:
    std::cerr << "Entrada inválida: " << std::endl;
    print_vec(std::vector<std::string>(argv + 1, argv + argc));
    std::cout << "Deve ser " << argv[0] << " <NUM_COZINHEIROS, NUM_CLIENTES>" << std::endl;

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

/* Print greeting message to the screen.
 */
inline void print_greeting() {
  std::vector<std::string> frenchy_firstnames{"Jean",     "Pierre",  "Jacques",  "Michel", "Sophie",
                                              "Isabelle", "Camille", "Nathalie", "Anne"};
  std::vector<std::string> frenchy_lastnames{"Renault", "Martin", "Laurent", "Moreau",  "Dubois",
                                             "Lefèvre", "Moreau", "Dupont",  "Fontaine"};

  std::string head_cook_name = generate_cook_name(frenchy_firstnames, frenchy_lastnames);

  std::cout << std::endl;
  std::cout << "*****************************************************************************" << std::endl;
  std::cout << std::endl;
  std::cout << "Bienvenue, monsieur ou madame, seja calorosamente bem-vindo ao" << std::endl;
  std::cout << "Chez L’Exagéré, o restaurante francês onde a elegância é obrigatória e o" << std::endl;
  std::cout << "cardápio, praticamente ilegível." << std::endl;
  std::cout << std::endl;
  std::cout << "Aqui, cada prato é uma obra de arte — e, convenhamos, o preço também." << std::endl;
  std::cout << std::endl;
  std::cout << "Nosso menu é elaborado diariamente — ou, como preferimos dizer," << std::endl;
  std::cout << "emocionalmente — pelo renomado chef " << head_cook_name << ", que acredita firmemente" << std::endl;
  std::cout << "que manteiga é um estado de espírito." << std::endl;
  std::cout << std::endl;
  std::cout << "*****************************************************************************" << std::endl;
  std::cout << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

/* Thread-safe implementation of stdout print (with timestamp)
 */
inline void thread_safe_print(const std::string &s, std::mutex &m) {
  std::lock_guard<std::mutex> lock(m);

  std::cout << "[" << system_time() << "] " << s << std::endl;
}

/* Returns random delay time between 0 and 9 seconds
 */
inline int get_random_delay_millis() { return (system_time() % 9) * 1000; }

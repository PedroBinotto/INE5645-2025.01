#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#define DEFAULT_N_1 3
#define DEFAULT_N_2 3

template <typename T> void print_list(const std::vector<T> &list, const std::string &sep = ", ") {
  for (size_t i = 0; i < list.size(); ++i)
    std::cout << list[i] << ((i == list.size() - 1) ? "\n" : sep);
}

int main(int argc, const char **argv) {
  int a, b;

  switch (argc) {
  case 1:
    std::cout << "`DEFAULT_N_1, DEFAULT_N_2` não informados; usando parâmetros padrão: " << std::endl;
    print_list<int>({DEFAULT_N_1, DEFAULT_N_2});

    a = DEFAULT_N_1;
    b = DEFAULT_N_1;
    break;
  case 3:
    std::cout << "Usando parâmetros informados: " << std::endl;
    print_list(std::vector<std::string>(argv + 1, argv + argc));

    a = std::stoi(argv[1]);
    b = std::stoi(argv[2]);
    break;
  default:
    std::cerr << "Entrada inválida: " << std::endl;
    print_list(std::vector<std::string>(argv + 1, argv + argc));
    std::cout << "Deve ser X, Y Z" << std::endl;

    exit(1);
  }
  // Perform actual operations

  return 0;
}

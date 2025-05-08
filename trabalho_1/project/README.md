# Trabalho 1

> _Definição do Trabalho 1: Programação Paralela_
>
> Este trabalho explora o uso de padrões para programação multithread. A adoção de padrões de projeto para programação paralela visa atender requisitos de desempenho, escalabilidade, extensibilidade, integração, dentre outros. O grupo deverá propor uma aplicação concorrente de sua preferência, que tenha requisitos relevantes para a aplicação dos padrões escolhidos. 
>
> Para o desenvolvimento desta aplicação, considere implementações concorrentes envolvendo a sincronização usando threads, processos ou coroutines. Além disso, será necessário utilizar estruturas de sincronização, como mutex, semáforos, barreiras e variáveis de condição. Não há restrição quanto a linguagem de programação utilizada, desde que a mesma explore adequadamente os aspectos de concorrência/paralelismo e sincronização necessários em sua aplicação

## Compilação e execução

### Pré-requisitos

- Este projeto utiliza a lib estática `pthreads`; se necessário, instalar através do gerenciador de pacotes do seu sistema operacional (ex.: Debian/Ubuntu):

```bash
sudo apt-get install libpthread-stubs0-dev
```

### Build

- O `Makefile` disponibilizado oferece targets para build, execução e cleanup:

```bash
# "make" sem especificar targets exibirá a mensagem de ajuda

pedro@machine ➜ project (main) make

 Choose a make command to run

  build   compile project to binary
  clean   clean up object and binary files
  run     build and run project
```

---

- `build`;

```bash
pedro@machine ➜ project (main) make build
g++ -std=c++11 -Wall -Wextra -g -pedantic -lncurses -pthread -c src/main.cpp -o obj/main.o
g++ -std=c++11 -Wall -Wextra -g -pedantic -lncurses -pthread obj/main.o -o bin/ticket -lncurses -pthread
```

---

- `run`;

```bash
pedro@machine ➜ project (main) make run
./bin/ticket
Hello!
```

---

- `clean`;

```bash
pedro@machine ➜ project (main) make clean
rm -rf obj bin log
```

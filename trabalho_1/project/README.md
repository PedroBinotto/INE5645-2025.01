# Trabalho 1

**[Self link](https://github.com/PedroBinotto/INE5645-2025.01/blob/main/trabalho_1/project/README.md)**

> **_Definição do Trabalho 1: Programação Paralela_**
>
> Este trabalho explora o uso de padrões para programação multithread. A adoção de padrões de projeto para programação paralela visa atender requisitos de desempenho, escalabilidade, extensibilidade, integração, dentre outros. O grupo deverá propor uma aplicação concorrente de sua preferência, que tenha requisitos relevantes para a aplicação dos padrões escolhidos. 
>
> Para o desenvolvimento desta aplicação, considere implementações concorrentes envolvendo a sincronização usando threads, processos ou coroutines. Além disso, será necessário utilizar estruturas de sincronização, como mutex, semáforos, barreiras e variáveis de condição. Não há restrição quanto a linguagem de programação utilizada, desde que a mesma explore adequadamente os aspectos de concorrência/paralelismo e sincronização necessários em sua aplicação

---

Link para o vídeo de apresentação do trabalho: [Google Docs](https://drive.google.com/drive/folders/116GsTjZRknR5FTEKkQzuvRdQMXof-LzJ?usp=drive_link)

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

  build         compile project to binary
  clean         clean up object and binary files
  run           build and run project;
  run $(ARGS)   run with command line args via `make run ARGS="<arg1, arg2 ...>"`
```

---

- `build`;

```bash
pedro@machine ➜ project (main) make build
g++ -std=c++11 -Wall -Wextra -g -pedantic -lncurses -pthread -c src/main.cpp -o obj/main.o
g++ -std=c++11 -Wall -Wextra -g -pedantic -lncurses -pthread obj/main.o -o bin/restaurant -lncurses -pthread
```

---

- `run`;

```bash
pedro@machine ➜ project (main) make run
./bin/restaurant
Hello!
```

---

- `run` (com parâmetros customizados);

O comando de `run` permite também ao usuário custamizar a paramtetrização do programa; as seguintes variáveis podem ser informadas ao executar o binário:

- `TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS`; _default: **15**;_
- `NUM_COZINHEIROS`; _default: **5**;_
- `NUM_CLIENTES`; _default: **30**;_
- `NUM_FOGOES`; _default: **2**;_
- `NUM_FORNOS`; _default: **2**;_

Através do Make, podem ser informados segundo à seguinte sitaxe:

```bash
pedro@machine ➜ project (main) make run ARGS="<arg1, arg2 ...>"
./bin/restaurant
Hello!
```

Em tal caso os argumentos devem ser informados na ordem correta:

```
1                2             3           4           5
NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS, TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS
```

Podem ser informados, de uma só vez:

```
<NUM_COZINHEIROS, NUM_CLIENTES> ou
<NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS> ou
<NUM_COZINHEIROS, NUM_CLIENTES, NUM_FOGOES, NUM_FORNOS, TEMPO_FECHAMENTO_RESTAURANTE_SEGUNDOS>
```

ex.:

```bash
pedro@machine ➜ project (main) make run ARGS="10 10"
./bin/restaurant 10 10
...

pedro@machine ➜ project (main) make run ARGS="10 10 20 20"
./bin/restaurant 10 10 20 20
...
```

---

- `clean`;

```bash
pedro@machine ➜ project (main) make clean
rm -rf obj bin log
```

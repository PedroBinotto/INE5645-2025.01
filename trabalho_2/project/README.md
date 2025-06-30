# Trabalho 2

**[Self link](https://github.com/PedroBinotto/INE5645-2025.01/blob/main/trabalho_2/project/README.md)**

> **_Definição do Trabalho 2: Programação Distribuída_**
>
> Este trabalho visa o desenvolvimento de uma aplicação distribuída com condições de corrida e necessidade de resolução de conflitos para garantia de consistência de dados. O objetivo é desenvolver um protótipo para a abstração de memória compartilhada distribuída, permitindo que o espaço de endereçamento, que é dado em nível de blocos, possa ser utilizado por todos os processos de um grupo.

---

Link para o vídeo de apresentação do trabalho: [Google Drive]()

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
mpic++ -std=c++17 -Wall -Wextra -g -pedantic  -c src/main.cpp -o obj/main.o
mpic++ -std=c++17 -Wall -Wextra -g -pedantic  obj/main.o -o bin/distributed
```

---

- `run`;

```bash
pedro@machine ➜ project (main) make run
mpirun -n 5 bin/distributed
Hello world from processor machine, rank 1 out of 5 processors
...
```

---

- `clean`;

```bash
pedro@machine ➜ project (main) make clean
rm -rf obj bin log
```

---

## Ambiente de desenvolvimento

O processo de configuração do LSP ([clangd](https://clangd.llvm.org/)) para adequadamente incluir os artefatos MPI para consulta no editor de texto, foi necessário gerar um arquivo `compile_commands.json` através da ferramenta [Bear](https://github.com/rizsotto/Bear):

```bash
# pwd: INE5645-2025.01/trabalhos/trabalho_2/project
bear -- make run
```

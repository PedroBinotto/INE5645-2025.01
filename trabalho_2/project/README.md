# Trabalho 2

**[Self link](https://github.com/PedroBinotto/INE5645-2025.01/blob/main/trabalho_2/project/README.md)**

> **_Definição do Trabalho 2: Programação Distribuída_**
>
> Este trabalho visa o desenvolvimento de uma aplicação distribuída com condições de corrida e necessidade de resolução de conflitos para garantia de consistência de dados. O objetivo é desenvolver um protótipo para a abstração de memória compartilhada distribuída, permitindo que o espaço de endereçamento, que é dado em nível de blocos, possa ser utilizado por todos os processos de um grupo.

---

Link para o vídeo de apresentação do trabalho: [Google Drive](https://drive.google.com/drive/folders/1rDEZZM1AuLYS2fbKXrLf-Uh9MKKvFWPk?usp=sharing)

## Compilação e execução

### Pré-requisitos

- Este projeto utiliza o framework [OpenMPI](https://www.open-mpi.org/); se necessário, instalar através do gerenciador de pacotes do seu sistema operacional (ex.: Debian/Ubuntu, dependências de dev: `libopenmpi-dev`):

```bash
sudo apt-get install openmpi libopenmpi-dev
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

- `run` (com parâmetros customizados);

O comando de `run` permite também ao usuário customizar a parametrização do programa; as seguintes variáveis podem ser informadas ao executar o binário:

- `BLOCK_SIZE`; _default: **8**;_
- `NUM_BLOCKS`; _default: **4**;_

Através do Make, podem ser informados segundo à seguinte sintaxe:

```bash
pedro@machine ➜ project (main) make run ARGS="<arg1, arg2 ...>"
./bin/distributed
Hello!
```

Em tal caso os argumentos devem ser informados na ordem correta:

```
1           2
BLOCK_SIZE, NUM_BLOCKS
```

Podem ser informados, de uma só vez:

```
<BLOCK_SIZE> ou
<BLOCK_SIZE, NUM_BLOCKS>
```

ex.:

```bash
pedro@machine ➜ project (main) make run ARGS="10"
mpirun -n 5 bin/distributed 10
...

pedro@machine ➜ project (main) make run ARGS="10 10"
mpirun -n 5 bin/distributed 10 10
...
```

---

- `clean`;

```bash
pedro@machine ➜ project (main) make clean
rm -rf obj bin log
```

---

### Mais configurações

Adicionalmente, também é possível variar o número de processos instanciados pelo MPI ou habilitar opções de _debug_ através do Makefile do projeto:

#### Número de processos

Por padrão, o comando `make run` produzirá 5 instâncias paralelas do programa (4 instâncias _worker_ + 1 instância _broadcaster_) através do MPI. Este atributo de configuração é determinado pela constante `N_PROCS` (o número total é dado por `N_PROCS + 1`), que está definida no topo do [Makefile](https://github.com/PedroBinotto/INE5645-2025.01/blob/750d370288b212725144c20224e400d81b9894b4/trabalho_2/project/Makefile) do projeto, em uma seção destinada à exploração por parte do usuário:

```make
# **********************************************************************************************************************
# "USER-LEVEL" CONFIGURATION SECTION BEGINS HERE! THESE VARIABLES ARE SAFE TO TWEAK FOR DIFFERENT BEHAVIOUR

# Determines how many intances of the process will be spawned by MPICC
N_PROCS := 4

# "USER-LEVEL" CONFIGURATION SECTION ENDS HERE! DO NOT EDIT CODE BEYOND THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING
# **********************************************************************************************************************
```

Ao alterar este valor, o comando `make run` apresentará diferenças no comportamento de acordo com a nova configuração.

#### Opções de _debug_

O Makefile incluído também oferece opções de _debug_ para que permitem executar o código com mais observabilidade (para desenvolvimento/estudo do programa). Para habilitar o modo de _debug_, basta _des-comentar_ a linha do arquivo que define as _flags_ usadas para depurar o programa; da mesma forma, pode-se alterar o nível de _logging_ do programa através do atributo `LOG_LEVEL` (os _logs_ serão direcionados ao `stdout` e também serão armazenados no diretório `logs/`):

```make
...

# Uncomment to enable DEBUG mode
DEBUG_OPTIONS := xterm -e gdb catch throw -ex "break std::terminate" -ex "run" --args
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

# Defines logging behaviour; must be one of:
# * 0 - Will log only the startup messages and errors
# * 1 - Will log operations such as READ, WRITE, MPI_Send, etc.
# * 2 - Will log operations and dump application state at every main thread iteration
LOG_LEVEL := 2
# ^^^^^^^^^^^^

# "USER-LEVEL" CONFIGURATION SECTION ENDS HERE! DO NOT EDIT CODE BEYOND THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING
# **********************************************************************************************************************
```

Por padrão, habilitar o modo de _debug_ instanciará uma janela de terminal executando o [GDB](https://www.sourceware.org/gdb/) para cada processo inicializado pelo MPI, mas este comportamento pode ser ajustado alterando os conteúdos do Makefile.

Para ajustes mais avançados, também é possível alterar os valores em `src/constants.hpp` para alterar o ritmo de execução das instruções (através do intervalo de "descanso" das threads), o número máximo de blocos ou o tamanho máximo dos blocos, por exemplo.

## Ambiente de desenvolvimento

O processo de configuração do LSP ([clangd](https://clangd.llvm.org/)) para adequadamente incluir os artefatos MPI para consulta no editor de texto, foi necessário gerar um arquivo `compile_commands.json` através da ferramenta [Bear](https://github.com/rizsotto/Bear):

```bash
# pwd: INE5645-2025.01/trabalho_2/project
bear -- make run
```

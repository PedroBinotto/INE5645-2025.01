#include "utils.hpp"
#include <mpi.h>
#include <stdio.h>

int main(int argc, const char **argv) {
  int world_size, world_rank, name_len;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Get_processor_name(processor_name, &name_len);

  const bool verbose = world_rank == 0;
  program_args params = capture_args(argc, argv, verbose);

  validate_args(params, world_size, verbose);

  MPI_Barrier(MPI_COMM_WORLD);

  std::cout << "Hello world from processor " << processor_name << ", rank "
            << world_rank << " out of " << world_size << " processors"
            << std::endl;

  MPI_Finalize();
}

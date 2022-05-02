#define _GNU_SOURCE

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  int rank, size, i;
  char *data,
      *recData;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  recData = malloc(100);
  for (i = 0; i < 100; i++)
    recData[i] = '\0';

  if (0 > asprintf(&data, "I am here %d!", rank))
    return EXIT_FAILURE;

  if (rank == 0)
  {
    // send to everyone
    for (i = 1; i < size; i++)
    {
      MPI_Send(data, strlen(data), MPI_CHAR, i, 0, MPI_COMM_WORLD);
    }
    // receive from everyone
    for (int i = 1; i < size; i++)
    {
      MPI_Recv(recData, 100, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%d Got back: %s\n",rank, recData);
    }
  }
  else if (rank > 0)
  {
    MPI_Recv(recData, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(data, strlen(data), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  }

  free(data);
  MPI_Finalize();

  return EXIT_SUCCESS;
}

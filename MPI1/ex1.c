#define _GNU_SOURCE

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int rank, size;
    char *data,
        *recData;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    recData = malloc(100);
    for (int i = 0; i < 100; i++)
        recData[i] = '\0';

    if (0 > asprintf(&data, "I am here %d!", rank))
        return EXIT_FAILURE;

    if (rank == 0)
    {
        printf("%d Sending out : %s\n", rank, data);
        MPI_Send(data, strlen(data), MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
        MPI_Recv(recData, 100, MPI_CHAR, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%d Got : %s\n", rank, recData);
    }
    else
    {
        MPI_Recv(recData, 100, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%d Got : %s\n", rank, recData);
        printf("%d Sending out : %s\n", rank, data);
        MPI_Send(data, strlen(data), MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
    }

    free(data);
    MPI_Finalize();

    return EXIT_SUCCESS;
}

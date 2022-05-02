#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main (int argc, char *argv[])
{
   int rank;
   char data[] = "I am here!",
        *recData;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);
   if (rank == 0)
      { printf ("Transmitted message: %s \n", data);
        MPI_Send (data, strlen (data), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
      }
      else if (rank == 1)
              { int i;
                recData = malloc (100);
                for (i = 0; i < 100; i++)
                  recData[i] = '\0';
                MPI_Recv (recData, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf ("Received message: %s \n", recData);
              }
   MPI_Finalize ();

   return EXIT_SUCCESS;
}

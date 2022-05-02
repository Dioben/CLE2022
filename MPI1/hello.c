#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
   int i, rank, size;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);
   MPI_Comm_size (MPI_COMM_WORLD, &size);
   printf ("Hello! I am %d of %d.\n", rank, size);
   if ((rank == 0) && (argc > 1))
      { for (i = 1; i < argc; i++)
          printf ("%s ", argv[i]);
        printf ("\n");
      }
   MPI_Finalize ();
   return EXIT_SUCCESS;
}

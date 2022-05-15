#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int rank, size;

    int *data,
        *recData,
        *minData,
        *maxData;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rcount = size*size;

    srand((unsigned int)time(NULL));

    data = malloc(sizeof(int) * rcount);
    if (rank == 0)
    {

        for (int i = 0; i < rcount; i++)
            data[i] = rand();

        printf("generated data:\n");

        for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++)
            printf("%10d ",data[i*size + j]);
        printf("\n");
        }
        printf("\n");

    }



    recData = malloc(sizeof(int) * rcount / size);
    MPI_Scatter(data, rcount / size, MPI_INT, recData, rcount/size, MPI_INT, 0, MPI_COMM_WORLD);
    minData = malloc(sizeof(int) * rcount / size);
    maxData = malloc(sizeof(int) * rcount / size);
    
    MPI_Reduce(recData, minData, rcount/size, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(recData, maxData, rcount/size, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank==0){
        printf("reduce round 1\n");
        for (int i = 0; i < rcount/size; i++)
            printf("Worker %d: min %d, max %d\n",i+1,minData[i],maxData[i]);
        printf("\n");
    }
    
    MPI_Scatter(minData, rcount / size / size , MPI_INT, recData, rcount / size/size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Reduce(recData, minData, rcount/size/size, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
     if (rank==0){
        printf("min report:%d\n",minData[0]);
    }
    MPI_Scatter(maxData, rcount / size / size , MPI_INT, recData, rcount / size/size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Reduce(recData, maxData, rcount/size/size, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
     if (rank==0){
        printf("max report:%d\n",maxData[0]);
    }

    free(data);
    free(recData);
    free(minData);
    free(maxData);
    MPI_Finalize();
    return EXIT_SUCCESS;
}

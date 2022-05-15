#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double calcProduct(double* m1,double* m2, int size, int idx){
    int row = idx / size; 
    int col = idx % size;
    printf("trying calc for %d %d,%d\n",idx,row,col);
    double val = 0;
    for (int i = 0;i<size;i++){

        val+= m1[size*row+i] * m2[size*i+col];
    }
    printf("finished calc for %d %d,%d\n",idx,row,col);
    return val;
}


int main(int argc, char *argv[])
{
    int rank, size;
    int matrixSize;
    double* matrix1; 
    double* matrix2;
    double* resultMatrix;


    srand((unsigned int)time(NULL));


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0)
    {
        matrixSize = 8;
        matrix1 = malloc(sizeof(double) * matrixSize * matrixSize);
        matrix2 = malloc(sizeof(double) * matrixSize * matrixSize);
        resultMatrix = malloc(sizeof(double) * matrixSize * matrixSize);
        MPI_Bcast(&matrixSize,1,MPI_INT,0,MPI_COMM_WORLD);
        for (int i = 0;i<matrixSize*matrixSize;i++){
            matrix1[i] = rand()%500;
            matrix2[i] = rand()%500;
        }
        MPI_Bcast(&matrix1,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);
        MPI_Bcast(&matrix2,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);
    }else{
        MPI_Bcast(&matrixSize,1,MPI_INT,0,MPI_COMM_WORLD);
        matrix1 = malloc(sizeof(double) * matrixSize * matrixSize);
        matrix2 = malloc(sizeof(double) * matrixSize * matrixSize);
        MPI_Bcast(&matrix1,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);
        MPI_Bcast(&matrix2,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);
    }

    int count = (matrixSize* matrixSize) /size;
    int floor =  count * rank;
    int ceiling =  count * (rank+1);
    double* localResults = malloc(sizeof(double)*count);
    printf("starting calc for %d %d\n",floor,ceiling);
    for (int i = floor;i<ceiling;i++){
        localResults[i] = calcProduct(matrix1,matrix2,matrixSize,i);
    } 
    printf("left calc for %d %d\n",floor,ceiling);
    MPI_Gather(localResults,count,MPI_DOUBLE,resultMatrix,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);
    if (rank==0){
        printf("Here be the results:\n");
        for (int i=0;i<matrixSize;i++){
            for (int j=0;j<matrixSize;j++)
                printf("%.2f ",resultMatrix[i*matrixSize+j]);
        printf("\n");
        }
        free(resultMatrix);
    }
    free(localResults);
    free(matrix1);
    free(matrix2);
    MPI_Finalize();
    return EXIT_SUCCESS;
}

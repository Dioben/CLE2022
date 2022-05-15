#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double calcProduct(double* m1,double* m2, int size, int idx){
    int row = idx / size; 
    int col = idx % size;
    double val = 0;
    for (int i = 0;i<size;i++){
        val+= m1[size*row+i] * m2[size*i+col];
    }
    return val;
}


int main(int argc, char *argv[])
{
    int rank, size;
    int matrixSize;
    double* matrix1; 
    double* matrix2;
    double* resultMatrix;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand((unsigned int)time(NULL));
    matrixSize = 8;
    matrix1 = malloc(sizeof(double) * matrixSize * matrixSize);
    matrix2 = malloc(sizeof(double) * matrixSize * matrixSize);

    if (rank == 0)
    {
        resultMatrix = malloc(sizeof(double) * matrixSize * matrixSize);
        
        for (int i = 0;i<matrixSize*matrixSize;i++){
            matrix1[i] = rand()%500;
            matrix2[i] = rand()%500;
        }
    }
    //init m1
    MPI_Bcast(matrix1,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);
    //init m2
    MPI_Bcast(matrix2,matrixSize*matrixSize,MPI_DOUBLE,0,MPI_COMM_WORLD);




    int count = (matrixSize* matrixSize) /size;
    int floor =  count * rank;
    int ceiling =  count * (rank+1);
    double* localResults = malloc(sizeof(double)*count);
    for (int i = floor;i<ceiling;i++){
        localResults[i] = calcProduct(matrix1,matrix2,matrixSize,i);
    } 
    MPI_Gather(localResults,count,MPI_DOUBLE,resultMatrix,count,MPI_DOUBLE,0,MPI_COMM_WORLD);
    
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

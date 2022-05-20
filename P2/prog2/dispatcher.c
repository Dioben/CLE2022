/**
 * @file dispatcher.c (implementation file)
 *
 * @brief Problem name: multiprocess determinant calculation
 *
 * Contains implementation of the dispatcher process.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "dispatcher.h"

int dispatchFileTasksRoundRobin(char* filename,int nextDispatch,int size, Result* result){
    FILE *file = fopen(filename, "rb");
    if (file == NULL){
        (*result).matrixCount = -1;
        return nextDispatch;
    }


    // number of matrices in the file
    int count;
    // order of the matrices in the file
    int order;

    fread(&count, 4, 1, file);
    fread(&order, 4, 1, file);
    double matrix[order*order];

    //init result struct
    (*result).matrixCount = count;
    (*result).determinants = malloc(sizeof(double) * count);


    MPI_Request req[size-1];
    for(int x=0;x<(size-1);x++)
        req[x] = MPI_REQUEST_NULL;
    //TODO: POSSIBLY IMPROVE, WITH 50 ISH SMALL FILES AND ONE WORKER THIS MAY RETURN NAN

    for (int i=0;i<count;i++){
        //read matrix from file
        fread(matrix, 8 , order*order, file);
        MPI_Wait(&req[nextDispatch-1], MPI_STATUS_IGNORE);
        //send order of next task, task
        MPI_Isend( &order , 1 , MPI_INT , nextDispatch , 0 , MPI_COMM_WORLD , &req[nextDispatch-1]);
        MPI_Request_free(&req[nextDispatch-1]);
        
        MPI_Isend( matrix , order*order , MPI_DOUBLE , nextDispatch , 0 , MPI_COMM_WORLD , &req[nextDispatch-1]);
        //advance dispatch number, wraps back to 1 after size
        nextDispatch++;
        if (nextDispatch>=size)
            nextDispatch=1;
    }

return nextDispatch;
}

void mergeChunks(int size, Result* results, int resultCount){
    int nextReceive = 1;
    //for each file
    for(int i=0;i<resultCount;i++){
        Result res = results[i];
        //for each determinant
        for (int k=0;k<res.matrixCount;k++){
            //get determinant
            MPI_Recv(res.determinants+k,1,MPI_DOUBLE,nextReceive,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            //avance dispatch
            nextReceive++;
            if(nextReceive>=size)
                nextReceive=1;
        }
    }

}
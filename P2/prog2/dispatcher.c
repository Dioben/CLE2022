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
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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


    MPI_Request req;
    for (int i=0;i<count;i++){
        //read matrix from file
        fread(matrix, 8 , order*order, file);
        //send order of next task, task
        MPI_Isend( &order , 1 , MPI_INT , nextDispatch , 0 , MPI_COMM_WORLD , &req);
        MPI_Request_free(&req);
        
        MPI_Isend( matrix , order*order , MPI_DOUBLE , nextDispatch , 0 , MPI_COMM_WORLD , &req);
        MPI_Request_free(&req);
        
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
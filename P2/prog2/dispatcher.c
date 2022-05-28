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
#include <pthread.h>

#include "dispatcher.h"
#include "sharedRegion.h"

void * dispatchFileTasksRoundRobin(){
    int nextDispatch = 1;
    for (int fIdx=0;fIdx<totalFileCount;fIdx++){
        char * filename = files[fIdx];
        FILE *file = fopen(filename, "rb");
        if (file == NULL){
            initResult(0);
        }


        // number of matrices in the file
        int count;
        // order of the matrices in the file
        int order;

        fread(&count, 4, 1, file);
        fread(&order, 4, 1, file);

        //init result struct
        initResult(count);

        for (int i=0;i<count;i++){
            //read matrix from file
            Task task = {.order = order, .matrix = malloc(sizeof(double)*order*order)};
            fread(task.matrix, 8 , order*order, file);
            pushTaskToSender(nextDispatch,task);
            //advance dispatch number, wraps back to 1 after size
            nextDispatch++;
            if (nextDispatch>=groupSize)
                nextDispatch=1;
        }


    }
    //send signal to stop workers
    Task stop = {.order = -1, .matrix = NULL};
    for(int i=1;i<groupSize;i++)
        pushTaskToSender(i,stop);
   
    
    pthread_exit((int *)EXIT_SUCCESS);
}

void* emitTasksToWorkers(){
    //curently employed entities
    bool working[groupSize-1];
    int currentlyWorking = groupSize-1;

    //request handler objects
    int requests[groupSize-1];
    for (int i=0;i<groupSize-1;i++){
        requests[i] = MPI_REQUEST_NULL;
    }
    int testStatus;
    while (currentlyWorking>0){
        
        for (int i=0;i<groupSize-1;i++){
            
            MPI_Test(requests[i],&testStatus,MPI_STATUS_IGNORE);
            if(working[i] && testStatus){
                //worker is ready
                //free() previous object
                //getTask (non blocking if empty)
                //if bad request set this thing to not working and send out a 0 to kill worker, decreaseCurrentlyWorking
                //set up sending this one
            }
            //if none of the previous ifs did anything check a smem method here
            // that method returns if updates have been posted since last call or blocks for a signal on next update
    }
    //wait for all the kill messages to have been sent

    }

    Task task = getTask()
    //MPI_Send( &order , 1 , MPI_INT , nextDispatch , 0 , MPI_COMM_WORLD);
            
    //MPI_Send( task.matrix , order*order , MPI_DOUBLE , nextDispatch , 0 , MPI_COMM_WORLD);
    int stop = 0;
    
    for (int i = 1; i < groupSize; i++)
        // signal that there's nothing left for workers to process
        MPI_Send(&stop, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
}

void * mergeChunks(){
    int nextReceive = 1;
    //for each file
    for(int i=0;i<totalFileCount;i++){
        Result* res = getResultToUpdate(i); //blocks until this results object has been initialized
        //for each determinant
        for (int k=0;k<(*res).matrixCount;k++){
            //get determinant
            MPI_Recv((*res).determinants+k,1,MPI_DOUBLE,nextReceive,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            //avance dispatch
            nextReceive++;
            if(nextReceive>=groupSize)
                nextReceive=1;
        }
    }

    pthread_exit((int *)EXIT_SUCCESS);

}
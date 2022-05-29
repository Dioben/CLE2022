/**
 * @file dispatcher.c (implementation file)
 *
 * @brief Problem name: multiprocess determinant calculation with multithreaded dispatcher
 *
 * Contains implementation of the dispatcher process threads.
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

/**
 * @brief Thread that reads file contents into local buffers so they can be sent to workers in a round-robin fashion.
 *
 * Will block when pushing chunks if it builds a significant lead over sender.
 *
 * @return pointer to the identification of this thread
 */
void *dispatchFileTasksIntoSender()
{
    int nextDispatch = 1;
    for (int fIdx = 0; fIdx < totalFileCount; fIdx++)
    {
        char *filename = files[fIdx];
        FILE *file = fopen(filename, "rb");

        // if file is a dud
        if (file == NULL)
        {
            initResult(0);
            continue;
        }

        // number of matrices in the file
        int count;
        fread(&count, 4, 1, file);

        // order of the matrices in the file
        int order;
        fread(&order, 4, 1, file);

        // init result struct
        initResult(count);

        for (int i = 0; i < count; i++)
        {
            // read matrix from file
            Task task = {.order = order, .matrix = malloc(sizeof(double) * order * order)};
            fread(task.matrix, 8, order * order, file);

            // send task into respective queue, this may block
            pushTaskToSender(nextDispatch, task);

            // advance dispatch number, wraps back to 1 after size
            nextDispatch++;
            if (nextDispatch >= processCount)
                nextDispatch = 1;
        }
    }

    // send signal to stop workers
    Task stop = {.order = -1, .matrix = NULL};
    for (int i = 1; i < processCount; i++)
        pushTaskToSender(i, stop);

    pthread_exit((int *)EXIT_SUCCESS);
}

/**
 * @brief Thread that emits chunks toward workers in a non-blocking manner.
 *
 * @return pointer to the identification of this thread
 */
void *emitTasksToWorkers()
{
    // curently employed entities
    bool working[processCount - 1];
    int currentlyWorking = processCount - 1;

    // request handler objects, last chunk received
    int requests[processCount - 1];
    Task tasks[processCount - 1];

    // init data for this function
    for (int i = 0; i < processCount - 1; i++)
    {
        requests[i] = MPI_REQUEST_NULL;
        tasks[i].order = 0;
        working[i] = true;
    }

    // used to signal workers that no more tasks are coming
    int killSignal = 0;

    // only ever used to receive waitAny result
    int lastFinish;

    int testStatus;

    while (true)
    {
        for (int i = 0; i < processCount - 1; i++)
        {
            MPI_Test(requests + i, &testStatus, MPI_STATUS_IGNORE);

            // if worker is ready
            if (working[i] && testStatus)
            {
                // clear out last task
                if (tasks[i].order > 0)
                    free(tasks[i].matrix);

                // try to get a new task
                if (!getTask(i, tasks + i))
                {
                    // task get failed, we use this to avoid free in future loops
                    tasks[i].order = 0;
                }
                else
                {
                    // if is a kill request
                    if (tasks[i].order == -1)
                    {
                        // signal worker to stop and mark this worker as dead
                        currentlyWorking--;
                        MPI_Isend(&killSignal, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, requests + i);
                        working[i] = false;
                        continue;
                    }
                    // send this task to worker in a non-blocking manner
                    MPI_Isend(&tasks[i].order, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, requests + i);
                    MPI_Request_free(requests + i);
                    MPI_Isend(tasks[i].matrix, tasks[i].order * tasks[i].order, MPI_DOUBLE, i + 1, 0, MPI_COMM_WORLD, requests + i);
                }
            }
        }
        if (currentlyWorking > 0)
        {
            // wait for any chunks to be available and for a worker to be available
            awaitFurtherTasks();
            MPI_Waitany(processCount - 1, requests, &lastFinish, MPI_STATUS_IGNORE);
        }
        else
            break;
    }

    // wait for all the kill messages to have been sent
    for (int i = 0; i < processCount - 1; i++)
        MPI_Wait(&requests[i], MPI_STATUS_IGNORE);

    pthread_exit((int *)EXIT_SUCCESS);
}

/**
 * @brief Thread that merges file chunks read by workers into their results structure.
 *
 * @return pointer to the identification of this thread
 */
void *mergeChunks()
{
    int nextReceive = 1;

    // for each file
    for (int i = 0; i < totalFileCount; i++)
    {
        // blocks until this results object has been initialized
        Result *res = getResultToUpdate(i);

        // for each determinant
        for (int k = 0; k < (*res).matrixCount; k++)
        {
            // get determinant
            MPI_Recv((*res).determinants + k, 1, MPI_DOUBLE, nextReceive, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // advance receive number, wraps back to 1 after processCount
            nextReceive++;
            if (nextReceive >= processCount)
                nextReceive = 1;
        }
    }

    pthread_exit((int *)EXIT_SUCCESS);
}
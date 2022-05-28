/**
 * @file sharedRegion.c (implementation file)
 *
 * @brief Problem name: multithreaded dispatcher for determinant calculation
 *
 * Shared region acessed by dispatcher and merger at the same time.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "sharedRegion.h"

/** @brief Number of files to be processed. */
int totalFileCount;

/** @brief Array with the file names of all files. */
char **files;

/** @brief Total process count. */
int groupSize;

/** @brief Index of last initialized results object. */
static int initializedResults;

/** @brief Array of the results for each file. */
static Result *results;

/** @brief Synchronization point when a new result object is initialized. */
static pthread_cond_t resultInitialized;

/** @brief Locking flag which warrants mutual exclusion while accessing the results array. */
static pthread_mutex_t resultsAccess = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Throws error and stops thread that threw.
 *
 * @param error error code
 * @param string error description
 */
static void throwThreadError(int error, char *string)
{
    errno = error;
    perror(string);
    pthread_exit((int *)EXIT_FAILURE);
}

/**
 * @brief Initializes the shared region.
 *
 * Needs to be called before anything else in this file.
 *
 * @param _totalFileCount number of files to be processed
 * @param _files array with the file names of all files
 */
void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount],int size)
{
    totalFileCount = _totalFileCount;
    files = _files;
    results = malloc(sizeof(Result) * totalFileCount);

    initializedResults = -1;
    pthread_cond_init(&resultInitialized, NULL);
    groupSize = size;
    
}

/**
 * @brief Frees all memory allocated during initialization of the shared region or the results.
 *
 * Should be called after the shared region has stopped being used.
 */
void freeSharedRegion()
{
    for (int i = 0; i < totalFileCount; i++)
        if (results[i].matrixCount>=0)
            free(results[i].determinants);
    free(results);
}

/**
 * @brief Initializes the result of a file.
 *
 * @param matrixCount number of matrices in the file
 */
void initResult(int matrixCount)
{

    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    results[++initializedResults].matrixCount = matrixCount;
    if (matrixCount>=0)
        results[initializedResults].determinants = malloc(sizeof(double) * matrixCount);

    if ((status = pthread_cond_signal(&resultInitialized)) != 0)
        throwThreadError(status, "Error on initResult() new result signal");

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");
}

/**
 * @brief Allows merger to get a result object, will block until result at index has been initialized
 *
 * @param fileIndex index of the file
 * @param matrixIndex index of the matrix in the file
 * @param determinant determinant of the matrix
 */
Result* getResultToUpdate(int idx)
{
    int status;//TODO: WHAT IF IDX TOO BIG

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on getResultToUpdate() lock");

    //wait for result initialization
    while(idx>initializedResults)
        if ((status = pthread_cond_wait(&resultInitialized, &resultsAccess)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on getResultToUpdate() unlock");
    return &results[idx];
}

/**
 * @brief Gets the results of all files.
 *
 * @return Result struct array with all the results
 */
Result *getResults()
{
    Result *val;
    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on getResults() lock");

    val = results;

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on getResults() unlock");

    return val;
}

/**
 * @brief Pushes a chunk to a given worker's queue
 * 
 * @param worker rank of worker chunk is meant for
 * @param task task that worker must perform
 */
void pushTaskToSender(int worker,Task task){
    //TODO: func
}

/**
 * @brief Get a task for a given worker
 * 
 * @param worker worker rank
 * @return Task* a task meant for the worker
 */
extern Task* getTask(int worker){
    //TODO: func
}


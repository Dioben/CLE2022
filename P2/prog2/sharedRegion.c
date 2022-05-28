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

/** @brief Max number of items the task FIFOs can contain. */
static int fifoSize;

/** @brief FIFOs with all the queued tasks. 1 per worker. */
static Task **taskFIFO;

/** @brief Insertion pointer to the task FIFO. */
static int *ii;

/** @brief Retrieval pointer to the task FIFO. */
static int *ri;

/** @brief Flag signaling the task FIFO is full. */
static bool *full;

/** @brief Synchronization point when a new result object is initialized. */
static pthread_cond_t resultInitialized;

/** @brief Locking flag which warrants mutual exclusion while accessing the results array. */
static pthread_mutex_t resultsAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Locking flag which warrants mutual exclusion while accessing the task FIFO. */
static pthread_mutex_t *fifoAccess;

/** @brief Locking flag to allow wait in awaitFurtherInfo. */
static pthread_mutex_t awaitAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Synchronization point when the task FIFO is full. */
static pthread_cond_t *fifoFull;

/** @brief Synchronization point when the task FIFO is empty. */
static pthread_cond_t fifoEmpty;

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
void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount],int size, int _fifoSize)
{
    totalFileCount = _totalFileCount;
    files = _files;
    fifoSize = _fifoSize;
    results = malloc(sizeof(Result) * totalFileCount);

    initializedResults = -1;
    pthread_cond_init(&resultInitialized, NULL);
    groupSize = size;
    pthread_cond_init(&fifoEmpty, NULL);
    
    ii = malloc(sizeof(int) * (groupSize-1));
    ri = malloc(sizeof(int) * (groupSize-1));
    full = malloc(sizeof(bool) * (groupSize-1));
    fifoAccess = malloc(sizeof(pthread_mutex_t) * (groupSize-1));
    fifoFull = malloc(sizeof(pthread_cond_t) * (groupSize-1));
    taskFIFO = malloc(sizeof(Task*) * (groupSize-1));
    for (int i = 0; i < (groupSize-1); i++) {
        ii[i] = 0;
        ri[i] = 0;
        full[i] = false;
        pthread_mutex_init(&fifoAccess[i], NULL);
        pthread_cond_init(&fifoFull[i], NULL);
        taskFIFO[i] = malloc(sizeof(Task) * fifoSize);
    }
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
    for (int i = 0; i < groupSize-1; i++){
        free(taskFIFO[i]);
    }
    free(taskFIFO);
    free(ii);
    free(ri);
    free(full);
    free(fifoAccess);
    free(fifoFull);
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
    worker--; // turns [1,groupSize] to [0,groupSize-1]
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on putTask() lock");

    while (full[worker])
        if ((status = pthread_cond_wait(&fifoFull[worker], &fifoAccess[worker])) != 0)
                throwThreadError(status, "Error on getTask() fifoEmpty wait");
    
    taskFIFO[worker][ii[worker]] = task;
    ii[worker] = (ii[worker] + 1) % fifoSize;
    full[worker] = (ii[worker] == ri[worker]);

    if ((status = pthread_mutex_unlock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on putTask() unlock");


    //notify that there's new content
    if ((status = pthread_mutex_lock(&awaitAccess)) != 0)
            throwThreadError(status, "Error on pushTaskToSender() notification lock");

    if ((status = pthread_cond_signal(&fifoEmpty)) != 0)
        throwThreadError(status, "Error on putTask() fifoEmpty signal");

    if ((status = pthread_mutex_unlock(&awaitAccess)) != 0)
        throwThreadError(status, "Error on pushTaskToSender() notification unlock");
}

/**
 * @brief Get a task for a given worker
 * 
 * @param worker worker rank minus 1
 * @return Task* a task meant for the worker
 */
bool getTask(int worker, Task *task){
    bool val = true;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on putTask() lock");
    // if not empty
    if (!(ii[worker] == ri[worker] && !full[worker]))
    {
        (*task).order = taskFIFO[worker][ri[worker]].order;
         (*task).matrix = taskFIFO[worker][ri[worker]].matrix;


        ri[worker] = (ri[worker] + 1) % fifoSize;
        full[worker] = false;

        if ((status = pthread_cond_signal(&fifoFull[worker])) != 0)
            throwThreadError(status, "Error on putTask() fifoEmpty signal");
    }
    else
    {
        val = false;
    }

    if ((status = pthread_mutex_unlock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on putTask() unlock");
    
    return val;
}

/**
 * @brief Block until there is pending data
 * 
 */
void awaitFurtherInfo()
{
    bool isEmpty = true;
    int status;
    if ((status = pthread_mutex_lock(&awaitAccess)) != 0)
            throwThreadError(status, "Error on awaitFurtherInfo() general lock");


    for (int i = 0; i < groupSize-1; i++)
    {
        if ((status = pthread_mutex_lock(&fifoAccess[i])) != 0)
            throwThreadError(status, "Error on awaitFurtherInfo() local lock");

        // if not empty
        if (!(ii[i] == ri[i] && !full[i]))
        {
            isEmpty = false;
            if ((status = pthread_mutex_unlock(&fifoAccess[i])) != 0)
                throwThreadError(status, "Error on awaitFurtherInfo() local unlock");
            break;
        }

        if ((status = pthread_mutex_unlock(&fifoAccess[i])) != 0)
            throwThreadError(status, "Error on awaitFurtherInfo() local unlock");
    }

    if (isEmpty)
    {
        if ((status = pthread_cond_wait(&fifoEmpty, &awaitAccess)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");
    }

     if ((status = pthread_mutex_unlock(&awaitAccess)) != 0)
            throwThreadError(status, "Error on awaitFurtherInfo() general unlock");
}

/**
 * @file sharedRegion.c (implementation file)
 *
 * @brief Problem name: multiprocess word count with multithreaded dispatcher
 *
 * Shared region acessed by dispatcher and merger threads at the same time.
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

/** @brief Total process count. Includes rank 0. */
int processCount;

/** @brief Index of last initialized results object. */
static int lastInitializedResult;

/** @brief Array of the results for each file. */
static Result *results;

/** @brief Max number of items the task FIFOs can contain. */
static int fifoSize;

/** @brief FIFOs with all the queued tasks. 1 per worker. */
static Task **taskFIFO;

/** @brief Insertion pointers to the task FIFOs. */
static int *ii;

/** @brief Retrieval pointers to the task FIFOs. */
static int *ri;

/** @brief Flags signaling the task FIFOs are full. */
static bool *full;

/** @brief Synchronization point when a new result object is initialized. */
static pthread_cond_t resultInitialized;

/** @brief Synchronization point when a chunk count is increased. */
static pthread_cond_t chunksIncreased;

/** @brief Locking flag which warrants mutual exclusion while accessing the results array. */
static pthread_mutex_t resultsAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Locking flags which warrant mutual exclusion while accessing the task FIFOs. */
static pthread_mutex_t *fifoAccess;

/** @brief Locking flag to allow wait in awaitFurtherTasks. */
static pthread_mutex_t awaitAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Synchronization points when the task FIFOs are full. */
static pthread_cond_t *fifoFull;

/** @brief Synchronization point when a new task is pushed. */
static pthread_cond_t newTask;

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
 * @param _processCount total process count
 * @param _fifoSize max number of items the task FIFOs can contain
 */
void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount], int _processCount, int _fifoSize)
{
    totalFileCount = _totalFileCount;
    files = _files;
    processCount = _processCount;
    fifoSize = _fifoSize;
    lastInitializedResult = -1;

    results = malloc(sizeof(Result) * totalFileCount);

    pthread_cond_init(&resultInitialized, NULL);
    pthread_cond_init(&chunksIncreased, NULL);
    pthread_cond_init(&newTask, NULL);

    // create a FIFO per worker
    ii = malloc(sizeof(int) * (processCount - 1));
    ri = malloc(sizeof(int) * (processCount - 1));
    full = malloc(sizeof(bool) * (processCount - 1));
    fifoAccess = malloc(sizeof(pthread_mutex_t) * (processCount - 1));
    fifoFull = malloc(sizeof(pthread_cond_t) * (processCount - 1));
    taskFIFO = malloc(sizeof(Task *) * (processCount - 1));
    for (int i = 0; i < (processCount - 1); i++)
    {
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
    free(results);
    for (int i = 0; i < processCount - 1; i++)
    {
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
 * @brief Initializes the result of the next file index.
 */
void initResult()
{

    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    results[++lastInitializedResult].chunks = 0;
    results[lastInitializedResult].consonantEndCount = 0;
    results[lastInitializedResult].vowelStartCount = 0;
    results[lastInitializedResult].wordCount = 0;

    if ((status = pthread_cond_signal(&resultInitialized)) != 0)
        throwThreadError(status, "Error on initResult() new result signal");

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");
}

/**
 * @brief Used when dispatcher has finished reading so that hasMoreChunks() can handle the last file.
 */
void finishedReading()
{
    int status;

    lastInitializedResult++;

    if ((status = pthread_cond_signal(&chunksIncreased)) != 0)
        throwThreadError(status, "Error on initResult() new result signal");
}

/**
 * @brief Increment the chunks read by 1.
 *
 * @param fileIndex index of the file
 */
void incrementChunks(int fileIndex)
{
    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    results[fileIndex].chunks++;

    if ((status = pthread_cond_signal(&chunksIncreased)) != 0)
        throwThreadError(status, "Error on initResult() new result signal");

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");
}

/**
 * @brief Allows merger to get a result object, will block until result at index has been initialized.
 *
 * @param fileIndex index of the file
 */
Result *getResultToUpdate(int fileIndex)
{
    if (fileIndex >= totalFileCount)
        return NULL;

    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on getResultToUpdate() lock");

    // wait for result initialization
    while (fileIndex > lastInitializedResult)
        if ((status = pthread_cond_wait(&resultInitialized, &resultsAccess)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on getResultToUpdate() unlock");
    return &results[fileIndex];
}

/**
 * @brief Will wait until there are more chunks to be used in the file or all chunks have already been used.
 *
 * @param fileIndex index of the file
 * @param currentChunk last chunk to be used
 * @return true if there are more chunks of the file available to be used
 * @return false if all chunks of the file have been used
 */
bool hasMoreChunks(int fileIndex, int currentChunk)
{
    bool val;
    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    // wait until there are chunks left in the current fileIndex
    // or it has been confirmed that no more chunks will be read
    while (fileIndex >= lastInitializedResult && currentChunk >= results[fileIndex].chunks)
        if ((status = pthread_cond_wait(&chunksIncreased, &resultsAccess)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");

    // if false, all chunks of current file index have been used
    val = currentChunk < results[fileIndex].chunks;

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    return val;
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
 * @brief Pushes a chunk to a given worker's queue.
 *
 * Notifies anyone inside awaitFurtherTasks.
 *
 * @param worker rank of worker chunk is meant for
 * @param task task that worker must perform
 */
void pushTaskToSender(int worker, Task task)
{
    worker--; // turns [1,processCount] to [0,processCount-1]
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on pushTaskToSender() lock");

    while (full[worker])
        if ((status = pthread_cond_wait(&fifoFull[worker], &fifoAccess[worker])) != 0)
            throwThreadError(status, "Error on pushTaskToSender() fifoFull wait");

    taskFIFO[worker][ii[worker]] = task;
    ii[worker] = (ii[worker] + 1) % fifoSize;
    full[worker] = (ii[worker] == ri[worker]);

    if ((status = pthread_mutex_unlock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on pushTaskToSender() unlock");

    // notify that there's new content
    if ((status = pthread_mutex_lock(&awaitAccess)) != 0)
        throwThreadError(status, "Error on pushTaskToSender() notification lock");

    if ((status = pthread_cond_signal(&newTask)) != 0)
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
bool getTask(int worker, Task *task)
{
    bool val = true;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on getTask() lock");

    // if not empty
    if (!(ii[worker] == ri[worker] && !full[worker]))
    {
        (*task).byteCount = taskFIFO[worker][ri[worker]].byteCount;
        (*task).bytes = taskFIFO[worker][ri[worker]].bytes;

        ri[worker] = (ri[worker] + 1) % fifoSize;
        full[worker] = false;

        if ((status = pthread_cond_signal(&fifoFull[worker])) != 0)
            throwThreadError(status, "Error on getTask() fifoFull signal");
    }
    else
    {
        val = false;
    }

    if ((status = pthread_mutex_unlock(&fifoAccess[worker])) != 0)
        throwThreadError(status, "Error on getTask() unlock");

    return val;
}

/**
 * @brief Block until there are pending tasks.
 */
void awaitFurtherTasks()
{
    bool isEmpty = true;
    int status;
    if ((status = pthread_mutex_lock(&awaitAccess)) != 0)
        throwThreadError(status, "Error on awaitFurtherInfo() general lock");

    for (int i = 0; i < processCount - 1; i++)
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
        if ((status = pthread_cond_wait(&newTask, &awaitAccess)) != 0)
            throwThreadError(status, "Error on awaitFurtherInfo() fifoEmpty wait");
    }

    if ((status = pthread_mutex_unlock(&awaitAccess)) != 0)
        throwThreadError(status, "Error on awaitFurtherInfo() general unlock");
}
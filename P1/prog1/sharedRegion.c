/**
 * @file sharedRegion.c (implementation file)
 *
 * @brief Problem name: multithreaded word count
 *
 * TODO: add description
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

/** @brief Number of files assigned to workers. */
static int assignedFileCount;

/** @brief Number of workers reading a file. */
static int readerCount;

/** @brief Array of the results for each file. */
static Result *results;

/** @brief Max number of items the task FIFO can contain. */
static int fifoSize;

/** @brief FIFO with all the queued tasks. */
static Task *taskFIFO;

/** @brief Insertion pointer to the task FIFO. */
static int ii;

/** @brief Retrieval pointer to the task FIFO. */
static int ri;

/** @brief Flag signaling the task FIFO is full. */
static bool full;

/** @brief Locking flag which warrants mutual exclusion while accessing the assignedFileCount variable. */
static pthread_mutex_t assignedFileCountAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Locking flag which warrants mutual exclusion while accessing the readerCount variable. */
static pthread_mutex_t readerCountAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Locking flag which warrants mutual exclusion while accessing the results array. */
static pthread_mutex_t resultsAccess = PTHREAD_MUTEX_INITIALIZER;

/** @brief Locking flag which warrants mutual exclusion while accessing the task FIFO. */
static pthread_mutex_t fifoAccess = PTHREAD_MUTEX_INITIALIZER;

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
 * @param _fifoSize max number of items the task FIFO can contain
 * @param workerCount number of workers accessing this shared region
 */
void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount], int _fifoSize, int workerCount)
{
    assignedFileCount = ii = ri = 0;
    full = false;

    totalFileCount = _totalFileCount;
    files = _files;
    fifoSize = _fifoSize;

    readerCount = workerCount;

    results = malloc(sizeof(Result) * totalFileCount);
    for (int i = 0; i < totalFileCount; i++)
    {
        results[i].vowelStartCount = 0;
        results[i].consonantEndCount = 0;
        results[i].wordCount = 0;
    }
    taskFIFO = malloc(sizeof(Task) * fifoSize);

    pthread_cond_init(&fifoEmpty, NULL);
}

/**
 * @brief Frees all memory allocated during initialization.
 *
 * Should be called after the shared region has stopped being used.
 */
void freeSharedRegion()
{
    free(results);
    free(taskFIFO);
}

/**
 * @brief Gets the index of the next file to be read.
 *
 * Autoincrements index on call.
 *
 * @return index of the file
 */
int getNewFileIndex()
{
    int val;
    int status;

    if ((status = pthread_mutex_lock(&assignedFileCountAccess)) != 0)
        return -1;

    val = assignedFileCount++;

    if ((status = pthread_mutex_unlock(&assignedFileCountAccess)) != 0)
        return -2;

    return val;
}

/**
 * @brief Informs shared region that the number of workers reading files has decreased.
 */
void decreaseReaderCount()
{
    int status;

    if ((status = pthread_mutex_lock(&readerCountAccess)) != 0)
        throwThreadError(status, "Error on decreaseReaderCount() lock");

    readerCount--;

    if (readerCount == 0)
        if ((status = pthread_cond_broadcast(&fifoEmpty)) != 0)
            throwThreadError(status, "Error on decreaseReaderCount() fifoEmpty broadcast");

    if ((status = pthread_mutex_unlock(&readerCountAccess)) != 0)
        throwThreadError(status, "Error on decreaseReaderCount() unlock");
}

/**
 * @brief Updates result of a file.
 *
 * @param fileIndex index of the file
 * @param result values used to update the result
 */
void updateResult(int fileIndex, Result result)
{
    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on updateResult() lock");

    results[fileIndex].vowelStartCount += result.vowelStartCount;
    results[fileIndex].consonantEndCount += result.consonantEndCount;
    results[fileIndex].wordCount += result.wordCount;

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on updateResult() unlock");
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
 * @brief Gets the next task from the FIFO.
 *
 * If FIFO is empty and there are no more workers reading files a Task struct with fileIndex=-1 is returned.
 *
 * @return Task struct
 */
Task getTask()
{
    Task val;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on getTask() lock");

    // trying to lock readerCountAccess doesn't work here, even if readerCount is stored in a tmp variable at the start of the func
    while ((ii == ri && !full) && readerCount > 0)
        if ((status = pthread_cond_wait(&fifoEmpty, &fifoAccess)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");

    // if not empty
    if (!(ii == ri && !full))
    {
        val = taskFIFO[ri];
        ri = (ri + 1) % fifoSize;
        full = false;
    }
    else
    {
        val.fileIndex = -1;
    }

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on getTask() unlock");

    return val;
}

/**
 * @brief Tries to put a new task into the FIFO.
 *
 * @param task Task struct to be put into the FIFO
 * @return if task was put into the FIFO
 */
bool putTask(Task task)
{
    bool val = true;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on putTask() lock");

    if (full)
    {
        val = false;
    }
    else
    {
        taskFIFO[ii] = task;
        ii = (ii + 1) % fifoSize;
        full = (ii == ri);

        if ((status = pthread_cond_signal(&fifoEmpty)) != 0)
            throwThreadError(status, "Error on putTask() fifoEmpty signal");
    }

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on putTask() unlock");

    return val;
}

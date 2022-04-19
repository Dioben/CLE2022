#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "sharedRegion.h"

int totalFileCount;
char **files;

static int assignedFileCount;
static int readerCount;
static Result *results;

static int fifoSize;
static Task *taskFIFO;
static int ii;
static int ri;
static bool full;

static pthread_mutex_t assignedFileCountAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t readerCountAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t resultsAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fifoAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fifoEmpty;

void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount], int _fifoSize, int workerCount)
{
    assignedFileCount = ii = ri = 0;
    full = false;

    totalFileCount = _totalFileCount;
    files = _files;
    fifoSize = _fifoSize;
    readerCount = workerCount;

    results = malloc(sizeof(Result) * totalFileCount);
    taskFIFO = malloc(sizeof(Task) * fifoSize);

    pthread_cond_init(&fifoEmpty, NULL);
}

void freeSharedRegion()
{
    for (int i = 0; i < totalFileCount; i++)
        free(results[i].determinants);
    free(results);
    free(taskFIFO);
}

void throwThreadError(int error, char *string)
{
    errno = error;
    perror(string);
    pthread_exit((int *)EXIT_FAILURE);
}

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

void initResult(int fileIndex, int matrixCount)
{
    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    results[fileIndex].matrixCount = matrixCount;
    results[fileIndex].determinants = malloc(sizeof(double) * matrixCount);

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on initResult() unlock");
}

void updateResult(int fileIndex, int matrixIndex, double determinant)
{
    int status;

    if ((status = pthread_mutex_lock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on updateResult() lock");

    results[fileIndex].determinants[matrixIndex] = determinant;

    if ((status = pthread_mutex_unlock(&resultsAccess)) != 0)
        throwThreadError(status, "Error on updateResult() unlock");
}

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

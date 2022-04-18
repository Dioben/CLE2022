#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

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
static int full;               // boolean

static pthread_mutex_t assignedFileCountAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t readerCountAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t resultsAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fifoAccess = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fifoFull;
static pthread_cond_t fifoEmpty;

void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount], int _fifoSize, int workerCount)
{
    assignedFileCount = ii = ri = 0;
    full = 0;

    totalFileCount = _totalFileCount;
    files = _files;
    fifoSize = _fifoSize;
    readerCount = (totalFileCount < workerCount) ? totalFileCount : workerCount;

    results = malloc(sizeof(Result) * totalFileCount);
    taskFIFO = malloc(sizeof(Task) * fifoSize);

    pthread_cond_init(&fifoFull, NULL);
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

int *getNewFileIndex()
{
    int *val = malloc(sizeof(int) * 2); // exit status and output
    val[0] = EXIT_FAILURE;
    val[1] = -1;
    int status;

    if ((status = pthread_mutex_lock(&assignedFileCountAccess)) != 0)
        return val;

    val[1] = assignedFileCount++;

    if ((status = pthread_mutex_unlock(&assignedFileCountAccess)) != 0)
        return val;

    val[0] = EXIT_SUCCESS;
    return val;
}

int getAssignedFileCount()
{
    int val;
    int status;

    if ((status = pthread_mutex_lock(&assignedFileCountAccess)) != 0)
        throwThreadError(status, "Error on getAssignedFileCount() lock");

    val = assignedFileCount;

    if ((status = pthread_mutex_unlock(&assignedFileCountAccess)) != 0)
        throwThreadError(status, "Error on getAssignedFileCount() unlock");

    return val;
}

int getReaderCount()
{
    int val;
    int status;

    if ((status = pthread_mutex_lock(&readerCountAccess)) != 0)
        throwThreadError(status, "Error on getReaderCount() lock");

    val = readerCount;

    if ((status = pthread_mutex_unlock(&readerCountAccess)) != 0)
        throwThreadError(status, "Error on getReaderCount() unlock");

    return val;
}

void decreaseReaderCount()
{
    int status;

    if ((status = pthread_mutex_lock(&readerCountAccess)) != 0)
        throwThreadError(status, "Error on getReaderCount() lock");

    readerCount--;

    if ((status = pthread_mutex_unlock(&readerCountAccess)) != 0)
        throwThreadError(status, "Error on getReaderCount() unlock");
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

int isTaskListEmpty()
{
    int val;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on isTaskListEmpty() lock");

    val = (ii == ri) && !full;

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on isTaskListEmpty() unlock");

    return val;
}

int isTaskListFull()
{
    int val;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on isTaskListFull() lock");

    val = full;

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on isTaskListFull() unlock");

    return val;
}

Task getTask()
{
    Task val;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on getTask() lock");

    while ((ii == ri) && !full)
        if ((status = pthread_cond_wait(&fifoEmpty, &fifoAccess)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");

    val = taskFIFO[ri];
    ri = (ri + 1) % fifoSize;
    full = 0;

    if ((status = pthread_cond_signal(&fifoFull)) != 0)
        throwThreadError(status, "Error on getTask() fifoFull signal");

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on getTask() unlock");

    return val;
}

void putTask(Task val)
{
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on putTask() lock");

    while (full)
        if ((status = pthread_cond_wait(&fifoFull, &fifoAccess)) != 0)
            throwThreadError(status, "Error on putTask() fifoFull wait");

    taskFIFO[ii] = val;
    ii = (ii + 1) % fifoSize;
    full = (ii == ri);

    if ((status = pthread_cond_signal(&fifoEmpty)) != 0)
        throwThreadError(status, "Error on putTask() fifoEmpty signal");

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on putTask() unlock");
}

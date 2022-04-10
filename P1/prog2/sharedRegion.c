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
static int full; // boolean
static int *resultInitialized; // boolean list

// tried separate mutex per variable but it segmentation faulted
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fifoFull;
static pthread_cond_t fifoEmpty;
static pthread_cond_t *resultInitializedCond;

void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount], int _fifoSize, int workerCount)
{
    assignedFileCount = ii = ri = 0;
    full = 0;

    totalFileCount = _totalFileCount;
    files = _files;
    fifoSize = _fifoSize;
    readerCount = (totalFileCount < workerCount) ? totalFileCount : workerCount;

    results = malloc(sizeof(Result)*totalFileCount);
    taskFIFO = malloc(sizeof(Task)*fifoSize);

    pthread_cond_init(&fifoFull, NULL);
    pthread_cond_init(&fifoEmpty, NULL);
    resultInitialized = malloc(sizeof(int)*totalFileCount);
    resultInitializedCond = malloc(sizeof(pthread_cond_t)*totalFileCount);
    for (int i = 0; i < totalFileCount; i++)
    {
        resultInitialized[i] = 0;
        pthread_cond_init(&resultInitializedCond[i], NULL);
    }
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
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on getNewFile() lock");
    
    val = assignedFileCount++;
    
    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on getNewFile() unlock");
    
    return val;
}

int getAssignedFileCount()
{
    int val;
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on getAssignedFileCount() lock");
    
    val = assignedFileCount;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on getAssignedFileCount() unlock");
    
    return val;
}

int getReaderCount()
{
    int val;
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on getReaderCount() lock");

    val = readerCount;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on getReaderCount() unlock");

    return val;
}

void decreaseReaderCount()
{
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on getReaderCount() lock");

    readerCount--;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on getReaderCount() unlock");
}

void initResult(int fileIndex, int matrixCount)
{
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on initResult() lock");

    results[fileIndex].matrixCount = matrixCount;
    results[fileIndex].determinants = malloc(sizeof(double)*matrixCount);

    resultInitialized[fileIndex] = 1;

    if ((status = pthread_cond_signal(&resultInitializedCond[fileIndex])) != 0)
        throwThreadError(status, "Error on initResult() resultInitializedCond signal");
    
    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on initResult() unlock");
}

void updateResult(int fileIndex, int matrixIndex, double determinant)
{
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on updateResult() lock");
    
    while (!resultInitialized[fileIndex])
        if ((status = pthread_cond_wait(&resultInitializedCond[fileIndex], &accessCR)) != 0)
            throwThreadError(status, "Error on updateResult() resultInitializedCond wait");
    
    results[fileIndex].determinants[matrixIndex] = determinant;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on updateResult() unlock");
}

Result *getResults()
{
    Result *val;
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on getResults() lock");
    
    val = results;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on getResults() unlock");

    return val;
}

int isTaskListEmpty()
{
    int val;
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on isTaskListEmpty() lock");

    val = (ii == ri) && !full;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on isTaskListEmpty() unlock");

    return val;
}

int isTaskListFull()
{
    int val;
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on isTaskListFull() lock");

    val = full;

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on isTaskListFull() unlock");

    return val;
}

Task getTask()
{
    Task val;
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on getTask() lock");

    while ((ii == ri) && !full)
        if ((status = pthread_cond_wait(&fifoEmpty, &accessCR)) != 0)
            throwThreadError(status, "Error on getTask() fifoEmpty wait");

    val = taskFIFO[ri];
    ri = (ri + 1) % fifoSize;
    full = 0;

    if ((status = pthread_cond_signal(&fifoFull)) != 0)
        throwThreadError(status, "Error on getTask() fifoFull signal");

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on getTask() unlock");

    return val;
}

void putTask(Task val)
{
    int status;
    
    if ((status = pthread_mutex_lock(&accessCR)) != 0)
        throwThreadError(status, "Error on putTask() lock");

    while (full)
        if ((status = pthread_cond_wait(&fifoFull, &accessCR)) != 0)
            throwThreadError(status, "Error on putTask() fifoFull wait");

    taskFIFO[ii] = val;
    ii = (ii + 1) % fifoSize;
    full = (ii == ri);

    if ((status = pthread_cond_signal(&fifoEmpty)) != 0)
        throwThreadError(status, "Error on putTask() fifoEmpty signal");

    if ((status = pthread_mutex_unlock(&accessCR)) != 0)
        throwThreadError(status, "Error on putTask() unlock");
}

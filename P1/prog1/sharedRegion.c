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

    readerCount = (totalFileCount < workerCount) ? totalFileCount : workerCount;

    results = malloc(sizeof(Result) * totalFileCount);
    for (int i = 0; i < totalFileCount; i++) {
        results[i].vowelStartCount = 0;
        results[i].consonantEndCount = 0;
        results[i].wordCount = 0;
    }
    taskFIFO = malloc(sizeof(Task) * fifoSize);

    pthread_cond_init(&fifoEmpty, NULL);
}

void freeSharedRegion()
{
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

bool isTaskListEmpty()
{
    bool val;
    int status;

    if ((status = pthread_mutex_lock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on isTaskListEmpty() lock");

    val = (ii == ri) && !full;

    if ((status = pthread_mutex_unlock(&fifoAccess)) != 0)
        throwThreadError(status, "Error on isTaskListEmpty() unlock");

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
    full = false;

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

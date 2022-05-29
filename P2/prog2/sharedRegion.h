/**
 * @file sharedRegion.h (interface file)
 *
 * @brief Problem name: multithreaded dispatcher for determinant calculation
 *
 * Shared region acessed by dispatcher, sender, and merger threads at the same time.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef SHARED_REGION_H_
#define SHARED_REGION_H_

#include <stdbool.h>

/**
 * @brief Struct containing the results calculated from a file.
 *
 * @param marixCount number of matrices in the file.
 * @param determinants array with the determinant of all matrices.
 */
typedef struct Result
{
    int matrixCount;
    double *determinants;
} Result;

/**
 * @brief Struct relative to a single task.
 *
 * @param order size of matrix
 * @param matrix pointer to matrix array of size order*order
 */
typedef struct Task
{
    int order;
    double *matrix;
} Task;

/** @brief Number of files to be processed. */
extern int totalFileCount;

/** @brief Array with the file names of all files. */
extern char **files;

/** @brief Total process count. Includes rank 0. */
extern int processCount;

/**
 * @brief Initializes the shared region.
 *
 * Needs to be called before anything else in this file.
 *
 * @param _totalFileCount number of files to be processed
 * @param _files array with the file names of all files
 * @param _processCount total process count
 * @param _fifoSize number of tasks that can be queued up for each worker
 */
extern void initSharedRegion(int _totalFileCount, char *_files[_totalFileCount], int _processCount, int _fifoSize);

/**
 * @brief Frees all memory allocated during initialization of the shared region or the results.
 *
 * Should be called after the shared region has stopped being used.
 */
extern void freeSharedRegion();

/**
 * @brief Initializes the result of the next file.
 *
 * @param matrixCount number of matrices in the file
 */
extern void initResult(int matrixCount);

/**
 * @brief Allows merger to get a result object, will block until result at index has been initialized.
 *
 * @param fileIndex index of the file
 */
extern Result *getResultToUpdate(int fileIndex);

/**
 * @brief Gets the results of all files.
 *
 * @return Result struct array with all the results
 */
extern Result *getResults();

/**
 * @brief Pushes a chunk to a given worker's queue.
 *
 * Notifies anyone inside awaitFurtherTasks.
 *
 * @param worker rank of worker chunk is meant for
 * @param task task that worker must perform
 */
extern void pushTaskToSender(int worker, Task task);

/**
 * @brief Get a task for a given worker
 *
 * @param worker worker rank minus 1
 * @param task a task meant for the worker
 * @return if getTask was successful (fifo was not empty)
 */
extern bool getTask(int worker, Task *task);

/**
 * @brief Block until there are pending tasks.
 */
extern void awaitFurtherTasks();

#endif
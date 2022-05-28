/**
 * @file sharedRegion.h (interface file)
 *
 * @brief Problem name: multithreaded dispatcher for determinant calculation
 *
 * Shared region acessed by dispatcher and merger threads at the same time.
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
 * "marixCount" - number of matrices in the file.
 * "determinants" - array with the determinant of all matrices.
 */
typedef struct Result
{
    int matrixCount;
    double *determinants;
} Result;

typedef struct Task
{
   int order;
   double *matrix;
}Task;

/** @brief Number of files to be processed. */
extern int totalFileCount;

/** @brief Array with the file names of all files. */
extern char **files;

/** @brief Total process count. */
extern int groupSize;
/**
 * @brief Initializes the shared region.
 *
 * Needs to be called before anything else in this file.
 *
 * @param _totalFileCount number of files to be processed
 * @param _files array with the file names of all files
 * @param workers number of available worker processes
 */
extern void initSharedRegion(int _totalFileCount, char *_files[], int workers, int _fifoSize);

/**
 * @brief Frees all memory allocated during initialization of the shared region or the results.
 *
 * Should be called after the shared region has stopped being used.
 */
extern void freeSharedRegion();


/**
 * @brief Initializes the result of a file.
 *
 * @param matrixCount number of matrices in the file
 */
extern void initResult(int matrixCount);


/**
 * @brief Gets the results of all files.
 *
 * @return Result struct array with all the results
 */
extern Result *getResults();

/**
 * @brief Allows merger to get a result object, will block until result at index has been initialized
 *
 * @param fileIndex index of the file
 * @param matrixIndex index of the matrix in the file
 * @param determinant determinant of the matrix
 */
extern Result* getResultToUpdate(int idx);

/**
 * @brief Pushes a chunk to a given worker's queue
 * 
 * @param worker rank of worker chunk is meant for
 * @param task task that worker must perform
 */
extern void pushTaskToSender(int worker,Task task);

/**
 * @brief Get a task for a given worker
 * 
 * @param worker worker rank
 * @param task a task meant for the worker
 * @return if getTask was successful (fifo was not empty)
 */
extern bool getTask(int worker, Task *task);

/**
 * @brief Block until there is pending data
 * 
 */
extern void awaitFurtherInfo()
#endif
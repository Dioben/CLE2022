/**
 * @file sharedRegion.h (interface file)
 *
 * @brief Problem name: multithreaded determinant calculation
 *
 * Shared region acessed by many threads at the same time.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

/**
 * @brief Struct containing the results calculated from a file.
 *
 * "matrixCount" - number of matrices in the file.
 * "determinants" - array with the determinant of all matrices.
 */
typedef struct Result
{
    int matrixCount;
    double *determinants;
} Result;


/**
 * @brief Parses a file into chunks, emits them via round robin
 *
 * @param nextDispatch, index to start dispatch on, is updated
 * @param dispatchedTotals tracks how many tasks have been sent to a worker
 * @param result has their internal variables set based on file contents
 */
extern void dispatchFileTasksRoundRobin(int* nextDispatch,int* dispatchedTotals , Result* result);

#endif
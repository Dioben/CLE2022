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
 * @param filename name of file to parse
 * @param nextDispatch index to start dispatch on
 * @param workerMax highest worker ID
 * @param result has their internal variables set based on file contents
 * 
 * @return next worker in line for task dispatch
 */
extern int dispatchFileTasksRoundRobin(char* filename,int nextDispatch,int workerMax, Result* result);

/**
 * @brief Merges task results from workers onto a results array
 *
 * @param workers number of workers
 * @param results results array
 * @param resultCount length of results array
 */
extern void mergeChunks(int workers, Result* results, int resultCount);

#endif
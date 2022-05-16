/**
 * @file worker.h (interface file)
 *
 * @brief Problem name: multithreaded determinant calculation
 *
 * Contains implementation of the worker threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef WORKER_H_
#define WORKER_H_

/**
 * @brief Worker thread.
 *
 * Its role is both to read files to generate tasks and to calculate results from tasks.
 *
 * @return pointer to the identification of this thread
 */
extern void *worker();

#endif
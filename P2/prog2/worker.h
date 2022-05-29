/**
 * @file worker.h (interface file)
 *
 * @brief Problem name: multiprocess determinant calculation with multithreaded dispatcher
 *
 * Contains implementation of the worker process.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef WORKER_H_
#define WORKER_H_

/**
 * @brief Worker process loop.
 *
 * Receives tasks via send and returns results from tasks.
 */
extern void whileTasksWorkAndSendResult();

#endif
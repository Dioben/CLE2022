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
 * @brief Parses a file into chunks, emits them towards the publisher
 */
extern void *dispatchFileTasksIntoSender();

/**
 * @brief Takes file chunks and sends them to workers
 */
extern void *emitTasksToWorkers();

/**
 * @brief Merges task results from workers onto a results array
 */
extern void *mergeChunks();

#endif
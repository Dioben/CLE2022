/**
 * @file dispatcher.h (interface file)
 *
 * @brief Problem name: multiprocess determinant calculation with multithreaded dispatcher
 *
 * Contains implementation of the dispatcher process threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

/**
 * @brief Thread that reads file contents into local buffers so they can be sent to workers in a round-robin fashion.
 *
 * Will block when pushing chunks if it builds a significant lead over sender.
 *
 * @return pointer to the identification of this thread
 */
extern void *dispatchFileTasksIntoSender();

/**
 * @brief Thread that emits chunks toward workers in a non-blocking manner.
 *
 * @return pointer to the identification of this thread
 */
extern void *emitTasksToWorkers();

/**
 * @brief Thread that merges file chunks read by workers into their results structure.
 *
 * @return pointer to the identification of this thread
 */
extern void *mergeChunks();

#endif
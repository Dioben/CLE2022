/**
 * @file dispatcher.h (interface file)
 *
 * @brief Problem name: multiprocess word count with multithreaded dispatcher
 *
 * Contains implementation of the dispatcher process.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

/**
 * @brief Thread that parses a file into chunks, emits them towards the publisher.
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
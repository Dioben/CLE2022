/**
 * @file sharedRegion.h (interface file)
 *
 * @brief Problem name: multiprocess word count with multithreaded dispatcher
 *
 * Shared region acessed by reader, sender, and merger at the same time.
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
 * @param chunks the number of chunks read from the file
 * @param vowelStartCount number of words that start with a vowel in the file
 * @param consonantEndCount number of words that end with a consonant in the file
 * @param wordCount number of words in the file
 */
typedef struct Result
{
    int chunks;
    int vowelStartCount;
    int consonantEndCount;
    int wordCount;
} Result;

/**
 * @brief Struct containing the data required for a worker to work on a task.
 *
 * @param byteCount number of bytes read from the file
 * @param bytes array with the bytes read from the file
 */
typedef struct Task
{
    int byteCount;
    char *bytes;
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
 */
extern void initResult();

/**
 * @brief Used when dispatcher has finished reading so that hasMoreChunks() can handle the last file.
 */
extern void finishedReading();

/**
 * @brief Increment the chunks read by 1.
 *
 * @param fileIndex index of the file
 */
extern void incrementChunks(int fileIndex);

/**
 * @brief Allows merger to get a result object, will block until result at index has been initialized.
 *
 * @param fileIndex index of the file
 */
extern Result *getResultToUpdate(int fileIndex);

/**
 * @brief Will wait until there are more chunks to be used in the file or all chunks have already been used.
 *
 * @param fileIndex index of the file
 * @param currentChunk last chunk to be used
 * @return if there are more chunks of the file to be used
 */
extern bool hasMoreChunks(int fileIndex, int currentChunk);

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
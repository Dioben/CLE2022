/**
 * @file sharedRegion.h (interface file)
 *
 * @brief Problem name: multithreaded word count
 *
 * Shared region acessed by many threads at the same time.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#ifndef SHARED_REGION_H_
#define SHARED_REGION_H_

#include <stdbool.h>

/**
 * @brief Struct containing the data required for a worker to work on a task.
 *
 * "fileIndex" - index of the file the data originates from.
 * "byteCount" - number of bytes read from the file.
 * "bytes" - array with the bytes read from the file.
 */
typedef struct Task
{
    int fileIndex;
    int byteCount;
    char *bytes;
} Task;

/**
 * @brief Struct containing the results calculated from a file.
 *
 * "vowelStartCount" - number of words that start with a vowel in the file.
 * "consonantEndCount" - number of words that end with a consonant in the file.
 * "wordCount" - number of words in the file.
 */
typedef struct Result
{
    int vowelStartCount;
    int consonantEndCount;
    int wordCount;
} Result;

/** @brief Number of files to be processed. */
extern int totalFileCount;

/** @brief Array with the file names of all files. */
extern char **files;

/**
 * @brief Initializes the shared region.
 *
 * Needs to be called before anything else in this file.
 *
 * @param _totalFileCount number of files to be processed
 * @param _files array with the file names of all files
 * @param _fifoSize max number of items the task FIFO can contain
 * @param workerCount number of workers accessing this shared region
 */
extern void initSharedRegion(int _totalFileCount, char *_files[], int _fifoSize, int workerCount);

/**
 * @brief Frees all memory allocated during initialization.
 *
 * Should be called after the shared region has stopped being used.
 */
extern void freeSharedRegion();

/**
 * @brief Gets the index of the next file to be read.
 *
 * Autoincrements index on call.
 *
 * @return index of the file
 */
extern int getNewFileIndex();

/**
 * @brief Informs shared region that the number of workers reading files has decreased.
 */
extern void decreaseReaderCount();

/**
 * @brief Updates result of a file.
 *
 * @param fileIndex index of the file which got an updated result
 * @param result values used to update the result
 */
extern void updateResult(int fileIndex, Result result);

/**
 * @brief Gets the results of all files.
 *
 * @return Result struct array with all the results
 */
extern Result *getResults();

/**
 * @brief Gets the next task from the FIFO.
 *
 * If FIFO is empty and there are no more workers reading files a Task struct with fileIndex=-1 is returned.
 *
 * @return Task struct
 */
extern Task getTask();

/**
 * @brief Tries to put a new task into the FIFO.
 *
 * @param task Task struct to be put into the FIFO
 * @return if task was put into the FIFO
 */
extern bool putTask(Task val);

#endif
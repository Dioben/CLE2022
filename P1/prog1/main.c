/**
 * @file main.c (implementation file)
 *
 * @brief Problem name: multithreaded word count
 *
 * Generator thread of the worker threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>

#include "worker.h"
#include "sharedRegion.h"

/**
 * @brief Struct containing the command line argument values.
 *
 * "status" - if the file was called correctly.
 * "fileCount" - count of the files given.
 * "fileNames" - array of file names given.
 * "workerCount" - count of the workers to be created.
 */
typedef struct CMDArgs
{
    int status;
    int fileCount;
    char **fileNames;
    int workerCount;
} CMDArgs;

/**
 * \brief Prints correct usage of this file
 *
 * \param cmdName name of the file
 */
static void printUsage(char *cmdName)
{
    fprintf(stderr, "\nSynopsis: %s OPTIONS [filename / positive number]\n"
                    "  OPTIONS:\n"
                    "  -h      --- print this help\n"
                    "  -f      --- file names, space separated\n"
                    "  -w      --- worker thread count (default: 2)\n",
            cmdName);
}

/**
 * @brief Processes the command line and returns a struct with the argument values.
 *
 * @param argc argument count
 * @param args argument array
 * @return CMDArgs struct with all the argument values
 */
CMDArgs parseCMD(int argc, char *args[])
{
    CMDArgs cmdArgs;
    cmdArgs.workerCount = 2;
    cmdArgs.status = EXIT_FAILURE;
    int opt;
    opterr = 0;
    unsigned int filestart = -1;
    unsigned int filespan = 0;

    if (argc == 1) // no args
    {
        fprintf(stderr, "%s: invalid format\n", basename(args[0]));
        printUsage(basename(args[0]));
        return cmdArgs;
    }
    do
    {
        switch ((opt = getopt(argc, args, "f:w:h")))
        {
        case 'f':                // file name
            if (filestart != -1) // duplicate -f
            {
                fprintf(stderr, "%s: -f can only be used once\n", basename(args[0]));
                printUsage(basename(args[0]));
                return cmdArgs;
            }
            filestart = optind - 1;
            for (filespan = 0; filestart + filespan < argc && args[filespan + filestart][0] != '-'; filespan++)
            {
                // constantly checks if within bounds and isnt next OPT
                // this loop only serves to advance filespan
            }
            cmdArgs.fileCount = filespan;
            cmdArgs.fileNames = (char **)malloc(sizeof(char **) * filespan);
            memcpy(cmdArgs.fileNames, &args[filestart], (sizeof(char *) * filespan));
            break;
        case 'w':
            cmdArgs.workerCount = atoi(optarg);
            if (cmdArgs.workerCount <= 0)
            {
                fprintf(stderr, "%s: non positive number\n", basename(args[0]));
                printUsage(basename(args[0]));
                if (!(filestart == -1 || filespan == 0))
                    free(cmdArgs.fileNames);
                return cmdArgs;
            }
            break;
        case 'h': // help
            printUsage(basename(args[0]));
            if (!(filestart == -1 || filespan == 0))
                free(cmdArgs.fileNames);
            return cmdArgs;
        case '?': // invalid option
            fprintf(stderr, "%s: invalid option\n", basename(args[0]));
            printUsage(basename(args[0]));
            if (!(filestart == -1 || filespan == 0))
                free(cmdArgs.fileNames);
            return cmdArgs;
        default: // -1
            break;
        }
    } while (opt != -1);
    if (filestart == -1 || filespan == 0) // no files
    {
        fprintf(stderr, "%s: file name is missing\n", basename(args[0]));
        printUsage(basename(args[0]));
        return cmdArgs;
    }
    cmdArgs.status = EXIT_SUCCESS;
    return cmdArgs;
}

/**
 * @brief Main thread.
 *
 * Its role is generating the worker threads, waiting for their termination, and printing the end result.
 *
 * @param argc argument count
 * @param args argument array
 * @return whether the main thread was executed correctly or not
 */
int main(int argc, char **args)
{
    CMDArgs cmdArgs = parseCMD(argc, args);
    if (cmdArgs.status == EXIT_FAILURE)
        return EXIT_FAILURE;

    int fileCount = cmdArgs.fileCount;
    char **fileNames = cmdArgs.fileNames;
    int workerCount = cmdArgs.workerCount;
    int fifoSize = workerCount * 10;

    double t = ((double)clock()) / CLOCKS_PER_SEC; // timer

    initSharedRegion(fileCount, fileNames, fifoSize, workerCount);

    pthread_t workers[workerCount];
    int i;

    for (i = 0; i < workerCount; i++)
    {
        if (pthread_create(&workers[i], NULL, worker, NULL) != 0)
        {
            perror("Error on creating worker threads");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < workerCount; i++)
    {
        if (pthread_join(workers[i], NULL) != 0)
        {
            perror("Error on waiting for worker threads");
            exit(EXIT_FAILURE);
        }
    }

    t = (((double)clock()) / CLOCKS_PER_SEC) - t;

    Result *results = getResults();
    printf("%-30s %15s %21s %21s\n", "File name", "Word count", "Starting with vowel", "Ending with consonant");
    for (i = 0; i < fileCount; i++)
    {
        printf("%-30s %15d %21d %21d\n", fileNames[i], results[i].wordCount, results[i].vowelStartCount, results[i].consonantEndCount);
    }

    freeSharedRegion();
    free(cmdArgs.fileNames);

    printf("\nElapsed time = %.6fs\n", t);

    exit(EXIT_SUCCESS);
}
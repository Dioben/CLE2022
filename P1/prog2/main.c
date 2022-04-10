#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>

#include "worker.h"
#include "sharedRegion.h"

typedef struct CMDArgs
{
    int status;
    int fileCount;
    char **fileNames;
    int workerCount;
} CMDArgs;

static void printUsage(char *cmdName)
{
    fprintf(stderr, "\nSynopsis: %s OPTIONS [filename / positive number]\n"
                    "  OPTIONS:\n"
                    "  -h      --- print this help\n"
                    "  -f      --- file names, space separated\n"
                    "  -w      --- worker thread count\n",
            cmdName);
}

CMDArgs cmdProcess(int argc, char *args[])
{
    CMDArgs cmdArgs;
    cmdArgs.workerCount = -1;
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
    if (cmdArgs.workerCount == -1)
    {
        fprintf(stderr, "%s: worker thread count is missing\n", basename(args[0]));
        printUsage(basename(args[0]));
        if (!(filestart == -1 || filespan == 0))
            free(cmdArgs.fileNames);
        return cmdArgs;
    }
    cmdArgs.status = EXIT_SUCCESS;
    return cmdArgs;
}

int main(int argc, char **args)
{
    CMDArgs cmdArgs = cmdProcess(argc, args);
    if (cmdArgs.status == EXIT_FAILURE)
        return EXIT_FAILURE;

    int fileCount = cmdArgs.fileCount;
    char **fileNames = cmdArgs.fileNames;
    int workerCount = cmdArgs.workerCount;
    int fifoSize = 64;

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

    Result *results = getResults();
    printf("%-50s %6s %30s\n", "File Name", "Matrix", "Determinant");
    for (i = 0; i < fileCount; i++)
    {
        for (int j = 0; j < results[i].matrixCount; j++)
        {
            printf("%-50s %6d %30.5e\n", fileNames[i], j + 1, results[i].determinants[j]);
        }
    }

    freeSharedRegion();
    free(cmdArgs.fileNames);

    exit(EXIT_SUCCESS);
}
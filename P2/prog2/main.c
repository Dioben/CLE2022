/**
 * @file main.c (implementation file)
 *
 * @brief Problem name: multithreaded determinant calculation
 *
 * Generator thread of the worker threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */
#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <time.h>

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
CMDArgs parseCMD(int argc, char *args[]) //TODO: REMOVE -W param
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
                fprintf(stderr, "%s: non positive worker count\n", basename(args[0]));
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

    int rank,size;

    MPI_Init(&argc, &args);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    if (size<2){
        print("There is a 2 process minimum\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }


    if (rank==0){ //dispatcher

        CMDArgs cmdArgs = parseCMD(argc, args);
        if (cmdArgs.status == EXIT_FAILURE){
            int stop = 0;
            for (int i=1;i<size;i++)
                //signal that there's nothing left to process
                MPI_Send(&stop, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Finalize();
            return EXIT_FAILURE;
        }
        int fileCount = cmdArgs.fileCount;
        char **fileNames = cmdArgs.fileNames;

        struct timespec start, finish; // time measurement
        clock_gettime(CLOCK_MONOTONIC_RAW, &start); // begin time measurement
        //for loop
        //getFIleInfo
        //dispatchTasksRoundRobin
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish); // end time measurement

        //TODO: POSSIBLY MOVE TO A PRINT RESULTS FUNCTION
        Result *results = getResults();
        printf("%-50s %6s %30s\n", "File name", "Matrix", "Determinant");
        for (i = 0; i < fileCount; i++)
        {
            for (int j = 0; j < results[i].matrixCount; j++)
            {
                printf("%-50s %6d %30.5e\n", fileNames[i], j + 1, results[i].determinants[j]);
            }
        }
        printf("\nElapsed time = %.6f s\n", (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
        free(cmdArgs.fileNames);
       
    }else{
        workWhileTasksAndReturnResult();
    }
    







    MPI_Finalize();
    exit(EXIT_SUCCESS);
}

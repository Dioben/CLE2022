#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "worker.h"
#include "sharedRegion.h"

int main(int argc, char **args)
{
    char *fileNames[] = {"../../02/dataset/mat128_32.bin",
                         "../../02/dataset/mat128_64.bin",
                         "../../02/dataset/mat128_128.bin",
                         "../../02/dataset/mat128_256.bin"};
    int fileCount = 4;
    int workerCount = 8;
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
            printf("%-50s %6d %30.5e\n", fileNames[i], j, results[i].determinants[j]);
        }
    }

    exit(EXIT_SUCCESS);
}
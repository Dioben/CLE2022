#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "worker.h"
#include "sharedRegion.h"

static void readMatrix(FILE *file, int order, double matrix[order][order])
{
    for (int x = 0; x < order; x++)
        for (int y = 0; y < order; y++)
            fread(&matrix[x][y], 8, 1, file);
}

static double calculateDeterminant(int order, double matrix[order][order])
{
    if (order == 1)
    {
        return matrix[0][0];
    }
    if (order == 2)
    {
        return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]; // AD - BC
    }
    double determinant = 1;

    for (int i = 0; i < order - 1; i++)
    { // triangular form
        for (int k = i + 1; k < order; k++)
        {
            double term = matrix[k][i] / matrix[i][i];
            for (int j = 0; j < order; j++)
            {
                matrix[k][j] = matrix[k][j] - term * matrix[i][j];
            }
        }
    }

    for (int i = 0; i < order; i++)
    { // multipy diagonals
        determinant *= matrix[i][i];
    }
    return determinant;
}

static void parseFile(int fileIndex)
{
    FILE *file = fopen(files[fileIndex], "rb");
    if (file == NULL)
        return;
    int count;
    int order;
    fread(&count, 4, 1, file);
    fread(&order, 4, 1, file);
    initResult(fileIndex, count);
    for (int i = 1; i <= count; i++)
    {
        double matrix[order][order];
        readMatrix(file, order, matrix);
        if (!isTaskListFull())
        {
            Task task = {.matrixIndex = i,
                         .fileIndex = fileIndex,
                         .order = order};
            task.matrix = malloc(sizeof(double)*order);
            for (int x = 0; x < order; x++)
                task.matrix[x] = malloc(sizeof(double)*order);
            memcpy(task.matrix, matrix, sizeof(double)*order*order);
            putTask(task);
        }
        else
        {
            double determinant = calculateDeterminant(order, matrix);
            updateResult(fileIndex, i, determinant);
        }
    }
}

void *worker(void *par)
{
    while (getAssignedFileCount() < totalFileCount)
    {
        // TODO: make it so if a file is not assigned it breaks instead of throwing
        int fileIndex = getNewFileIndex();
        parseFile(fileIndex);
    }
    decreaseReaderCount();
    while (!isTaskListEmpty() || getReaderCount()>0)
    {
        Task task = getTask();
        double matrix[task.order][task.order];
        memcpy(matrix, task.matrix, sizeof(double)*task.order*task.order);
        double determinant = calculateDeterminant(task.order, matrix);
        updateResult(task.fileIndex, task.matrixIndex, determinant);
    }
    
    pthread_exit((int *)EXIT_SUCCESS);
}
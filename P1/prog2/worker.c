#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "worker.h"
#include "sharedRegion.h"

static void readMatrix(FILE *file, int order, double** matrix)
{
    for (int x = 0; x < order; x++)
        for (int y = 0; y < order; y++)
            fread(matrix+x*order+y, 8, 1, file);
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

    // triangular form
    for (int i = 0; i < order - 1; i++)
    {
        if (matrix[i][i] == 0)
        {
            int foundJ = 0;
            for (int j = i + 1; j < order; j++)
                if (matrix[j][i] != 0)
                    foundJ = j;
            if (!foundJ)
                return 0;
            determinant *= -1;
            double tempRow[order];
            memcpy(tempRow, matrix[i], sizeof(double) * order);
            memcpy(matrix[i], matrix[foundJ], sizeof(double) * order);
            memcpy(matrix[foundJ], tempRow, sizeof(double) * order);
        }
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
    for (int i = 0; i < count; i++)
    {
        double ** matrix = malloc(sizeof(double) * order * order);
        readMatrix(file, order, matrix);
        Task task = {.matrixIndex = i,
                     .fileIndex = fileIndex,
                     .order = order,
                     .matrix = matrix};
        if (!putTask(task))
        {
            double determinant = calculateDeterminant(order, matrix);
            updateResult(fileIndex, i, determinant);
            free(task.matrix);
        }
    }
}

void *worker(void *par)
{
    int fileIndex;
    while ((fileIndex = getNewFileIndex()) < totalFileCount)
    {
        if (fileIndex < 0)
        {
            if (fileIndex == -1)
                perror("Error on getNewFileIndex() lock");
            else
                perror("Error on getNewFileIndex() unlock");
            continue;
        }
        parseFile(fileIndex);
    }
    decreaseReaderCount();
    Task task;
    while ((task = getTask()).fileIndex != -1)
    {
        double determinant = calculateDeterminant(task.order, task.matrix);
        free(task.matrix);
        updateResult(task.fileIndex, task.matrixIndex, determinant);
    }

    pthread_exit((int *)EXIT_SUCCESS);
}
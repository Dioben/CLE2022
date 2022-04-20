/**
 * @file worker.c (implementation file)
 *
 * @brief Problem name: multithreaded determinant calculation
 *
 * Contains implementation of the worker threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "worker.h"
#include "sharedRegion.h"

/**
 * @brief Reads a matrix from a file stream.
 *
 * @param file file stream
 * @param order order of the matrix
 * @param matrix 1D representation of the matrix to read into
 */
static void readMatrix(FILE *file, int order, double *matrix)
{
    for (int x = 0; x < order; x++)
        for (int y = 0; y < order; y++)
            fread(matrix + x * order + y, 8, 1, file);
}

/**
 * @brief Calculates the determinant of a matrix through Gaussian elimination.
 *
 * @param order order of the matrix
 * @param matrix 1D representation of the matrix
 * @return determinant of the matrix
 */
static double calculateDeterminant(int order, double *matrix)
{
    // if matrix is small do a simpler calculation
    if (order == 1)
    {
        return matrix[0];
    }
    else if (order == 2)
    {
        return matrix[0] * matrix[3] - matrix[1] * matrix[2]; // AD - BC
    }
    double determinant = 1;

    // turn matrix into a triangular form
    for (int i = 0; i < order - 1; i++)
    {
        // if diagonal is 0 swap rows with another whose value in that column is not 0
        if (matrix[i * order + i] == 0)
        {
            int foundJ = 0;
            for (int j = i + 1; j < order; j++)
                if (matrix[j * order + i] != 0)
                    foundJ = j;
            if (!foundJ)
                return 0;
            determinant *= -1;
            double tempRow[order];
            memcpy(tempRow, matrix + i * order, sizeof(double) * order);
            memcpy(matrix + i * order, matrix + foundJ * order, sizeof(double) * order);
            memcpy(matrix + foundJ * order, tempRow, sizeof(double) * order);
        }

        // gaussian elimination
        for (int k = i + 1; k < order; k++)
        {
            double term = matrix[k * order + i] / matrix[i * order + i];
            for (int j = 0; j < order; j++)
            {
                matrix[k * order + j] = matrix[k * order + j] - term * matrix[i * order + j];
            }
        }
    }

    // multiply diagonals of the triangular matrix
    for (int i = 0; i < order; i++)
    {
        determinant *= matrix[i * order + i];
    }
    return determinant;
}

/**
 * @brief Uses a file to create tasks.
 *
 * @param fileIndex index of the file
 */
static void parseFile(int fileIndex)
{
    FILE *file = fopen(files[fileIndex], "rb");
    if (file == NULL)
        return;

    // number of matrices in the file
    int count;
    fread(&count, 4, 1, file);

    // order of the matrices in the file
    int order;
    fread(&order, 4, 1, file);

    initResult(fileIndex, count);
    for (int i = 0; i < count; i++)
    {
        double *matrix = malloc(sizeof(double) * order * order);
        readMatrix(file, order, matrix);
        Task task = {.matrixIndex = i,
                     .fileIndex = fileIndex,
                     .order = order,
                     .matrix = matrix};

        // if FIFO is full process task instead
        if (!putTask(task))
        {
            double determinant = calculateDeterminant(order, matrix);
            updateResult(fileIndex, i, determinant);
            free(task.matrix);
        }
    }
}

/**
 * @brief Worker thread.
 *
 * Its role is both to read files to generate tasks and to calculate results from tasks.
 *
 * @return pointer to the identification of this thread
 */
void *worker()
{
    int fileIndex;

    // get a file and create tasks
    while ((fileIndex = getNewFileIndex()) < totalFileCount)
    {
        if (fileIndex < 0)
        {
            if (fileIndex == -1)
            {
                errno = EDEADLK;
                perror("Error on getNewFileIndex() lock");
            }
            else
            {
                errno = EDEADLK;
                perror("Error on getNewFileIndex() unlock");
            }
            continue;
        }
        parseFile(fileIndex);
    }
    decreaseReaderCount();

    Task task;

    // while there are tasks process them
    while ((task = getTask()).fileIndex != -1)
    {
        double determinant = calculateDeterminant(task.order, task.matrix);
        free(task.matrix);
        updateResult(task.fileIndex, task.matrixIndex, determinant);
    }

    pthread_exit((int *)EXIT_SUCCESS);
}
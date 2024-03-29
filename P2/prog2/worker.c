/**
 * @file worker.c (implementation file)
 *
 * @brief Problem name: multiprocess determinant calculation with multithreaded dispatcher
 *
 * Contains implementation of the worker process.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "worker.h"

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
 * @brief Worker process loop.
 *
 * Receives tasks via send and returns results from tasks.
 */
void whileTasksWorkAndSendResult()
{
    int matrixOrder;
    double *matrix;
    int currentMax = 0; // how much memory we've allocated to the matrix

    MPI_Request req = MPI_REQUEST_NULL;
    while (true)
    {
        // receive next task matrix size
        MPI_Recv(&matrixOrder, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // signal to stop working
        if (matrixOrder < 1)
        {
            // wait for last response to be read before shutdown
            MPI_Wait(&req, MPI_STATUS_IGNORE);
            break;
        }

        // our current matrix buffer isnt large enough
        if (matrixOrder > currentMax)
        {
            // if matrix has not been allocated yet
            if (currentMax == 0)
                matrix = malloc(sizeof(double) * matrixOrder * matrixOrder);
            else
                matrix = realloc(matrix, sizeof(double) * matrixOrder * matrixOrder);

            currentMax = matrixOrder;
        }

        // receive matrix
        MPI_Recv(matrix, matrixOrder * matrixOrder, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // calculate result
        double determinant = calculateDeterminant(matrixOrder, matrix);

        // wait for last send to cleared
        if (req != MPI_REQUEST_NULL)
            MPI_Wait(&req, MPI_STATUS_IGNORE);

        // send back result
        MPI_Isend(&determinant, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &req);
    }

    if (currentMax > 0)
        free(matrix);
}
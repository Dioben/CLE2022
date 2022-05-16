/**
 * @file sharedRegion.c (implementation file)
 *
 * @brief Problem name: multithreaded determinant calculation
 *
 * Shared region acessed by many threads at the same time.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "sharedRegion.h"



/**
 * @brief Initializes the result of a file.
 *
 * @param fileIndex index of the file
 * @param matrixCount number of matrices in the file
 */
void initResult(int fileIndex, int matrixCount)
{
    results[fileIndex].matrixCount = matrixCount;
    results[fileIndex].determinants = malloc(sizeof(double) * matrixCount);
}

/**
 * @brief Updates result of a file
 *
 * @param fileIndex index of the file
 * @param matrixIndex index of the matrix in the file
 * @param determinant determinant of the matrix
 */
void updateResult(int fileIndex, int matrixIndex, double determinant)
{
    results[fileIndex].determinants[matrixIndex] = determinant;
}
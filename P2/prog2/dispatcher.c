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


/**
 * @brief Reads a matrix from a file stream.
 *
 * @param file file stream
 * @param order order of the matrix
 * @param matrix 1D representation of the matrix to read into
 */
static void readMatrix(FILE *file, int order, double *matrix)
{
    fread(matrix, 8 , order*order, file); //TODO: MAYBE JUST REMOVE THIS
}


/**
 * @brief Uses a file to create tasks.
 *
 * @param fileName name of the file
 */
static void parseFile(char* fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
        return;

    // number of matrices in the file
    int count;
    fread(&count, 4, 1, file);

    // order of the matrices in the file
    int order;
    fread(&order, 4, 1, file);

    // return count, order, fp
    //TODO: stfuf
}

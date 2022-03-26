#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>

/* allusion to internal functions */
static void printUsage(char *cmdName);

/*
Gets file and order of the matrix
Returns the matrix
*/
void readMatrix(FILE *file, int order, double matrix[order][order])
{
    for (int x = 0; x < order; x++)
        for (int y = 0; y < order; y++)
            fread(&matrix[x][y], 8, 1, file);
}

/*
Gets order of the matrix and matrix
Returns the determinant of the matrix

Supports easy resolve for small matrixes, alternatively runs Gaussian elimination
*/
double calculateDeterminant(int order, double matrix[order][order])
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

/*
Given a file name, returns a list with the determinants of matrices in it
File format:
first 4 bytes -> int with number of matrices
second 4 bytes -> in with order of the matrices
rest of the file in 8 byte sections -> doubles in the matrices
*/
double *parseFile(char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
        return NULL;
    unsigned int count;
    unsigned int order;
    fread(&count, 4, 1, file);
    fread(&order, 4, 1, file);
    double *determinants = (double *)malloc((count + 1) * sizeof(double));
    determinants[0] = count;
    if (determinants == NULL)
    {
        perror("Failed to malloc.\n");
        exit(1);
    }
    for (int i = 1; i <= count; i++)
    {
        double matrix[order][order];
        readMatrix(file, order, matrix);
        determinants[i] = calculateDeterminant(order, matrix);
    }
    return determinants;
}

int main(int argc, char *args[])
{
    int opt;    /* selected option */
    opterr = 0; // this seems to set error throwing to manual (getopt can now return ?)
    unsigned int filestart = -1;
    unsigned int filespan = 0;

    do
    {
        switch ((opt = getopt(argc, args, "f:n:h")))
        {
        case 'f': /* file name */
            filestart = optind - 1;
            for (filespan = 0; filestart + filespan < argc && args[filespan + filestart][0] != '-'; filespan++)
            {
                // constantly checks if within bounds and isnt next OPT
                // this loop only serves to advance filespan
            }
            break;
        case 'h': /* help mode */
            printUsage(basename(args[0]));
            return EXIT_SUCCESS;
        case '?': /* invalid option */
            fprintf(stderr, "%s: invalid option\n", basename(args[0]));
            printUsage(basename(args[0]));
            return EXIT_FAILURE;
        case -1:
            break;
        }
    } while (opt != -1);

    if (argc == 1) // no args
    {
        fprintf(stderr, "%s: invalid format\n", basename(args[0]));
        printUsage(basename(args[0]));
        return EXIT_FAILURE;
    }
    if (filestart == -1 || filespan == 0) // no files
    {
        fprintf(stderr, "%s: file name is missing\n", basename(args[0]));
        printUsage(basename(args[0]));
        return EXIT_FAILURE;
    }

    double t0, t1, t2; /* time limits */
    t2 = 0.0;

    char *file;
    printf("%-50s %6s %30s\n", "File Name", "Matrix", "Determinant");
    for (int i = 0; i < filespan; i++)
    {
        file = args[filestart + i];

        t0 = ((double)clock()) / CLOCKS_PER_SEC;
        double *determinants = parseFile(file);
        t1 = ((double)clock()) / CLOCKS_PER_SEC;
        t2 += t1 - t0;
        int count = determinants[0];
        for (int ii = 1; ii <= count; ii++)
        {
            printf("%-50s %6d %30.5e\n", file, ii, determinants[ii]);
        }
        free(determinants);
    }

    printf("\nElapsed time = %.6f s\n", t2);
    return 0;
}

/**
 *  \brief Print command usage.
 *
 *  A message specifying how the program should be called is printed.
 *
 *  \param cmdName string with the name of the command
 */
static void printUsage(char *cmdName)
{
    fprintf(stderr, "\nSynopsis: %s OPTIONS [filename / positive number]\n"
                    "  OPTIONS:\n"
                    "  -h      --- print this help\n"
                    "  -f      --- filenames, space separated\n",
            cmdName);
}
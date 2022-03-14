#include <stdio.h>
#include <stdlib.h>

/*
Gets file and order of the matrix
Returns the matrix
*/
void readMatrix(FILE *file, int order, double matrix[order][order])
{
    for (int x = 0; x < order; x++)
        for (int y = 0; y < order; y++)
            fread(&matrix[x][y], sizeof(double *), 1, file);
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
    return -69.420;
    // TODO: write way to calc determinant for matrices with order > 2
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
    double *determinants = (double *) malloc((count + 1) * sizeof(double *));
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

int main(int argc, char **args)
{
    printf("%-50s %6s %30s\n", "File Name", "Matrix", "Determinant");
    for (int i = 1; i < argc; i++)
    {
        double *determinants = parseFile(args[i]);
        int count = determinants[0];
        for (int ii = 1; ii <= count; ii++)
        {
            printf("%-50s %6d %30.5f\n", args[i], ii, determinants[ii]);
        }
        free(determinants);
    }
    return 0;
}
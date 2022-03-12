#include <stdio.h>


/*
save current pointer
aim to 0
find first line with actual stuff on it, count elements
reset pointer to previous value
return size
*/
int readDimension(FILE* file){
    return 1;
}
/*
Scan first line for length, THEN declare stuff and read into matrix, return
*/
double ** readMatrix(char* file){
    FILE *file = fopen(file, "rb");
    if (file == NULL){
        int matrix[1][1];
        matrix[0][0]=0;
        return matrix;
    }
    int size = readDimension(file);
    double matrix[size][size];
    //TODO: ACTUALLY READ MATRIX
    return matrix;
}
/*
Supports easy resolve for small matrixes, alternatively runs Gaussian elim
*/
double calculateDeterminant(double ** matrix){
 size_t rows = sizeof(matrix)/sizeof(matrix[0]);
 if (rows == 1){
     return matrix[0][0];
 }
  if (rows == 2){
     return matrix[0][0]*matrix[1][1]- matrix[0][1]*matrix[1][0]; //AD - BC
 }
}

int main(int argc, char **args)
{
    printf("%-50s %30s\n", "File Name","Determinant");
    for (int i = 1; i < argc; i++)
    {
        double ** matrix = readMatrix(args[i]);
        double determinant = calculateDeterminant(matrix);
        printf("%-50s %30.5f\n", args[i],determinant);
    }

    return 0;
}
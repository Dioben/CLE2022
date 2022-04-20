#ifndef SHARED_REGION_H_
#define SHARED_REGION_H_

#include <stdbool.h>

typedef struct Task
{
    int fileIndex;
    int matrixIndex;
    int order;
    double **matrix;
} Task;

typedef struct Result
{
    int matrixCount;
    double *determinants;
} Result;

extern int totalFileCount;
extern char **files;

extern void initSharedRegion(int _totalFileCount, char *_files[], int _fifoSize, int workerCount);
extern void freeSharedRegion();
extern int getNewFileIndex();
extern void decreaseReaderCount();
extern void initResult(int fileIndex, int matrixCount);
extern void updateResult(int fileIndex, int matrixIndex, double determinant);
extern Result *getResults();
extern Task getTask();
extern bool putTask(Task val);

#endif
#ifndef SHARED_REGION_H_
#define SHARED_REGION_H_

typedef struct Task
{
    int fileIndex;
    int byteCount;
    char *bytes;
} Task;

typedef struct Result
{
    int vowelStartCount;
    int consonantEndCount;
    int wordCount;
} Result;

extern int totalFileCount;
extern char **files;

extern void initSharedRegion(int _totalFileCount, char *_files[], int _fifoSize, int workerCount);
extern void freeSharedRegion();
extern void throwThreadError(int error, char *string);
extern int *getNewFileIndex();
extern int getAssignedFileCount();
extern int getReaderCount();
extern void decreaseReaderCount();
extern void updateResult(int fileIndex, Result result);
extern Result *getResults();
extern int isTaskListEmpty();
extern int isTaskListFull();
extern Task getTask();
extern void putTask(Task val);

#endif
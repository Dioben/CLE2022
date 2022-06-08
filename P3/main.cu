/**
 * @file main.cu (implementation file)
 *
 * @brief Problem name: CUDA matrix multiplication (TODO: DID THIS TURN OUT ROW FIRST OR COL FIRST)
 *
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */


#include "common.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <time.h>

/**
 * @brief We know our hardware supports 1024 concurrent threads
 * 
 */
static const int MAX_THREADS = 1024;

/**
 * @brief Struct containing the command line argument values.
 *
 * @param status if the file was called correctly
 * @param fileCount count of the files given
 * @param fileNames array of file names given
 */
typedef struct CMDArgs
{
    int status;
    int fileCount;
    char **fileNames;
} CMDArgs;

/**
 * @brief Struct containing the results calculated from a file.
 *
 * @param marixCount number of matrices in the file.
 * @param determinants array with the determinant of all matrices.
 */
typedef struct Result
{
    int matrixCount;
    double *determinants;
} Result;

/**
 * @brief Prints correct usage of this file.
 *
 * @param cmdName name of the file
 */
static void printUsage(char *cmdName)
{
    fprintf(stderr, "\nSynopsis: %s OPTIONS [filenames]\n"
                    "  OPTIONS:\n"
                    "  -h      --- print this help\n"
                    "  -f      --- file names, space separated\n",
            cmdName);
}

/**
 * @brief Processes the command line and returns a struct with the argument values.
 *
 * @param argc argument count
 * @param args argument array
 * @return CMDArgs struct with all the argument values
 */
CMDArgs parseCMD(int argc, char *args[])
{
    CMDArgs cmdArgs;
    cmdArgs.status = EXIT_FAILURE;
    int opt;
    opterr = 0;
    unsigned int filestart = -1;
    unsigned int filespan = 0;

    if (argc == 1) // no args
    {
        fprintf(stderr, "%s: invalid format\n", basename(args[0]));
        printUsage(basename(args[0]));
        return cmdArgs;
    }
    do
    {
        switch ((opt = getopt(argc, args, "f:w:h")))
        {
        case 'f':                // file name
            if (filestart != -1) // duplicate -f
            {
                fprintf(stderr, "%s: -f can only be used once\n", basename(args[0]));
                printUsage(basename(args[0]));
                return cmdArgs;
            }
            filestart = optind - 1;
            for (filespan = 0; filestart + filespan < argc && args[filespan + filestart][0] != '-'; filespan++)
            {
                // constantly checks if within bounds and isnt next OPT
                // this loop only serves to advance filespan
            }
            cmdArgs.fileCount = filespan;
            cmdArgs.fileNames = (char **)malloc(sizeof(char **) * filespan);
            memcpy(cmdArgs.fileNames, &args[filestart], (sizeof(char *) * filespan));
            break;
        case 'h': // help
            printUsage(basename(args[0]));
            if (!(filestart == -1 || filespan == 0))
                free(cmdArgs.fileNames);
            return cmdArgs;
        case '?': // invalid option
            fprintf(stderr, "%s: invalid option\n", basename(args[0]));
            printUsage(basename(args[0]));
            if (!(filestart == -1 || filespan == 0))
                free(cmdArgs.fileNames);
            return cmdArgs;
        default: // -1
            break;
        }
    } while (opt != -1);
    if (filestart == -1 || filespan == 0) // no files
    {
        fprintf(stderr, "%s: file name is missing\n", basename(args[0]));
        printUsage(basename(args[0]));
        return cmdArgs;
    }
    cmdArgs.status = EXIT_SUCCESS;
    return cmdArgs;
}

/**
 * @brief Prints program results.
 *
 * @param fileNames names of processed files
 * @param fileCount how many files were processed
 */
static void printResults(char **fileNames, int fileCount, Result* results)
{
    printf("%-50s %6s %30s\n", "File name", "Matrix", "Determinant");
    for (int i = 0; i < fileCount; i++)
    {
        for (int j = 0; j < results[i].matrixCount; j++)
        {
            printf("%-50s %6d %30.5e\n", fileNames[i], j + 1, results[i].determinants[j]);
        }
    }
}


// grid 1D block 1D
__global__ void calculateDeterminantsOnGPU(double *matrix, double * determinants, int order, int offset, int totalMatrices)
{
    unsigned int idx = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned short localMatrix = idx/order;
    unsigned short row = idx%order;
    double hold;

    //initialize relevant shared memory
    if (idx<blockDim.x && idx+offset<totalMatrices)
        determinants[idx+offset] = 1;


    for (short i=0;i<order;i++){
            //TODO:ASSESS SWAP, SYNC
            //TODO: PERFORM SWAP, SYNC
            //TODO: REDUCTION, MULT HELD VALUE, SYNC
    }


}

static void parseFile(char * fileName, Result* resultSlot){

    FILE *file = fopen(fileName, "rb");
    // if file is a dud
    if (file == NULL)
    {
        (*resultSlot).matrixCount = 0;
        return;
    }
    // number of matrices in the file
    int count;
    fread(&count, 4, 1, file);

    // order of the matrices in the file
    int order;
    fread(&order, 4, 1, file);
    if (order>MAX_THREADS){
        printf("File %s with matrixes of size %d is larger than our limit %d, it will be ignored\n",fileName,count,MAX_THREADS);
        fclose(file);
        (*resultSlot).matrixCount = 0;
        return;

    }
    
    //initialize results object
    (*resultSlot).matrixCount = count;
    (resultSlot)->determinants = (double *) malloc(sizeof(double)*count);
    double * determinantsOnGPU;
    CHECK(cudaMalloc((void **)&determinantsOnGPU, sizeof(double)*count));
    memset(determinantsOnGPU,1,sizeof(double)*count);
    //how many matrixes we can work with at once
    double simultaneousMatrixes = MAX_THREADS/order;

    int memsize = MAX_THREADS * order* sizeof(double);
    double * matrixOnGPU;
    double * matrix = (double *) malloc(memsize);
    CHECK(cudaMalloc((void **)&matrixOnGPU, memsize));
    dim3 block(order, 1);
    dim3 grid((MAX_THREADS + order - 1) / order);
    
    for (int i = 0; i < count/simultaneousMatrixes; i++)
        {
            fread(matrix, 8, memsize, file);
            CHECK(cudaMemcpy(matrixOnGPU, matrix, memsize, cudaMemcpyHostToDevice));
            calculateDeterminantsOnGPU<<<grid, block>>>(matrixOnGPU, determinantsOnGPU, order, i*simultaneousMatrixes,count);
            CHECK(cudaDeviceSynchronize());
            CHECK(cudaGetLastError());
            
        }
    //copy results out of device
    CHECK(cudaMemcpy(determinantsOnGPU, (*resultSlot).determinants , count , cudaMemcpyDeviceToHost));
    
    //free memory
    CHECK(cudaFree(determinantsOnGPU));
    CHECK(cudaFree(matrixOnGPU));
    free(matrix);
    fclose(file);
}


int main(int argc, char **argv)
{

    struct timespec start, finish; // time measurement

    CMDArgs cmdArgs = parseCMD(argc, argv);
    if (cmdArgs.status == EXIT_FAILURE)
        return EXIT_FAILURE;

    // set up device
    int dev = 0;
    cudaDeviceProp deviceProp;
    CHECK(cudaGetDeviceProperties(&deviceProp, dev));
    printf("Using Device %d: %s\n", dev, deviceProp.name);
    CHECK(cudaSetDevice(dev));
    
    Result * results = (Result *) malloc( sizeof(Result)*cmdArgs.fileCount);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start); // begin time measurement

    for (int i =0;i<cmdArgs.fileCount;i++){
        parseFile(cmdArgs.fileNames[i],results+i);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish); // end time measurement

    printResults(cmdArgs.fileNames,cmdArgs.fileCount,results);
    printf("\nElapsed time = %.6f s\n", (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
    
    free(cmdArgs.fileNames);
    for (int i =0;i<cmdArgs.fileCount;i++){
        if (results[i].matrixCount)
            free(results[i].determinants);
    }
    free(results);
    
    CHECK(cudaDeviceReset());

    return (0);

}


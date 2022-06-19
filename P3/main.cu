/**
 * @file main.cu (implementation file)
 *
 * @brief Problem name: CUDA matrix multiplication along rows
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
    int filestart = -1;
    int filespan = 0;

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


/**
 * @brief Function responsible for computing determinants on GPU
 * 
 * @param matrix pointer containing all matrixes
 * @param determinants pointer containing determinant slots
 * @param order size of matrices
 * @return
 */
__global__ void calculateDeterminantsOnGPU(double *matrix, double * determinants, int order)
{
    //matrix we are working with
    unsigned int bx = blockIdx.x + gridDim.x * blockIdx.y + gridDim.x* gridDim.y* blockIdx.z;
    //row we are working with
    unsigned int idx = threadIdx.x+ blockDim.x* threadIdx.y + blockDim.x * blockDim.y * threadIdx.z;

    //point at our matrix
    matrix+=bx*order*order;

    //point at our row
    //matrix+= idx*order;

    double hold;

    //initialize relevant shared memory
    if (idx == 0){
        determinants[bx] = 1;
    }
    
    for (short i=0;i<order;i++){
        if (matrix[i*order+i]==0){
            
            short foundJ = 0;
            for (short j = i + 1; j < order; j++) 
                if (matrix[order * i + j] != 0) //this searches COLUMNS
                    foundJ = j;
                    break;
            
            if (!foundJ){ //no swap possible
                if (idx==0){
                    determinants[bx]=0; //set value before exit
                }
                return;
            }

            __syncthreads(); //SYNC POINT: WE KNOW WHAT SWAP IS REQUIRED
            
            if (idx>=i){
            //perform swap by grabbing value from row ROW, column FOUNDJ into row ROW column I
            hold = matrix[idx*order + foundJ];
            matrix[idx*order + foundJ] =  matrix[idx*order + i];
            matrix[idx*order + i] = hold;
            }


            __syncthreads(); //SYNC POINT: SWAPS HAVE BEEN PERFORMED
            if (idx==i){
                determinants[bx]*=-1;
            }
        }
        if (idx==i){
                determinants[bx]*=matrix[idx*order+idx];
            }
        if (idx>i){
            //REDUCE ALONG ROW
            hold = matrix[idx*order + i]/matrix[i*order+i]; //A(k,i) /A(i,i)
            for (int j = i; j < order; j++)
            {
                matrix[idx*order + j] -= hold * matrix[i* order + j];
            }
        }

        __syncthreads(); //SYNC POINT: REDUCE IS DONE
            
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

    if (order*order*count+count>(size_t) 5e9){
        printf("File %s is bigger than we can handle, it will be ignored\n",fileName);
        fclose(file);
        (*resultSlot).matrixCount = 0;
        return;

    }
    
    //initialize results object
    (*resultSlot).matrixCount = count;
    (*resultSlot).determinants = (double *) malloc(sizeof(double)*count);
    double * determinantsOnGPU;
    CHECK(cudaMalloc((void **)&determinantsOnGPU, sizeof(double)*count));

    int memsize = order * order * count * sizeof(double);
    
    double * matrixOnGPU;
    double * matrix = (double *) malloc(memsize);
    CHECK(cudaMalloc((void **)&matrixOnGPU, memsize));
    
    dim3 block(order, 1);
    dim3 grid(count);
    

    fread(matrix, 8, memsize, file);
    CHECK(cudaMemcpy(matrixOnGPU, matrix, memsize, cudaMemcpyHostToDevice));
    calculateDeterminantsOnGPU<<<grid, block>>>(matrixOnGPU, determinantsOnGPU, order);
    CHECK(cudaDeviceSynchronize());
    CHECK(cudaGetLastError());
            
    //copy results out of device
    CHECK(cudaMemcpy((*resultSlot).determinants ,determinantsOnGPU,  count*sizeof(double) , cudaMemcpyDeviceToHost));
    
    //free memory
    CHECK(cudaFree(determinantsOnGPU));
    CHECK(cudaFree(matrixOnGPU));
    fclose(file);
    free(matrix);
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


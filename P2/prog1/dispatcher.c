/**
 * @file dispatcher.c (implementation file)
 *
 * @brief Problem name: multiprocess word count
 *
 * Contains implementation of the dispatcher process.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include "dispatcher.h"
#include "utfUtils.h"

/**
 * @brief Reads a chunk from a file stream into a byte array.
 *
 * @param file file stream to be read
 * @return Task struct with the number of bytes read and byte array
 */

/** @brief The initial max number of bytes of the text chunk in a task. */
static const int MAX_BYTES_READ = 500;

/** @brief How many bytes the first text read leaves empty. */
static const int BYTES_READ_BUFFER = 50;


/**
 * @brief Reads an UTF-8 character from a file stream.
 *
 * @param file stream of the file to be read
 * @return UTF-8 character
 */
int readLetterFromFile(FILE *file)
{
    int letter = 0;
    if (fread(&letter, 1, 1, file) != 1)
        return EOF;

    // how many extra bytes need to be read after the first to get a full character
    // if -1 initial byte is invalid
    int loops = -1 + byte0utf8(letter) + 2 * byte1utf8(letter) + 3 * byte2utf8(letter) + 4 * byte3utf8(letter);
    if (loops < 0)
    {
        errno = EINVAL;
        perror("Invalid text found");
        return EOF;
    }

    for (int i = 0; i < loops; i++)
    {
        letter <<= 8;
        fread(&letter, 1, 1, file);
    }

    return letter;
}

static Task readBytes(FILE *file)
{
    Task task = {.byteCount = -1,
                 .bytes = malloc(sizeof(char) * MAX_BYTES_READ)};
    task.byteCount = fread(task.bytes, 1, (MAX_BYTES_READ - BYTES_READ_BUFFER), file);

    // if initial fread didn't read expected number of bytes it means it reached EOF
    if (task.byteCount != MAX_BYTES_READ - BYTES_READ_BUFFER)
    {
        task.bytes[task.byteCount] = EOF;
        return task;
    }

    // if initial fread ended in the middle of a character, add enough bytes to
    // make sure task.bytes ends at the end of a character
    while (true)
    {
        // how many extra bytes need to be read after the first to get a full character
        // if -1 byte is in the middle of a character
        int loops = -1 + byte0utf8(task.bytes[task.byteCount - 1]) + 2 * byte1utf8(task.bytes[task.byteCount - 1]) + 3 * byte2utf8(task.bytes[task.byteCount - 1]) + 4 * byte3utf8(task.bytes[task.byteCount - 1]);
        if (loops >= 0)
            task.byteCount += fread(task.bytes + task.byteCount, 1, loops, file);
        else
        {
            // as current byte is in the middle of a character read another one
            // and check again if its not in the middle

            // if EOF, should never happen in a valid UTF-8 file
            if (fread(task.bytes + task.byteCount, 1, 1, file) != 1)
                break;
            task.byteCount++;
            continue;
        }
        break;
    }

    int letter;

    // used to realloc byte array if needed
    int localMaxBytes = MAX_BYTES_READ;

    // read characters until the byte array doesn't end in the middle of a word
    do
    {
        // if byte array is almost full, increase its size
        if (task.byteCount >= localMaxBytes - 10)
        {
            localMaxBytes += 100;
            task.bytes = (char *)realloc(task.bytes, localMaxBytes);
        }

        letter = readLetterFromFile(file);

        // how many bytes this letter uses
        int loops = ((letter & 0xff000000) != 0) + ((letter & 0xffff0000) != 0) + ((letter & 0xffffff00) != 0);

        // store character bytes into byte array
        for (int i = loops; i >= 0; i--)
        {
            task.bytes[task.byteCount++] = letter >> (8 * i);
        }
    } while (!(isSeparator(letter) || isBridge(letter) || letter == EOF));

    task.bytes[task.byteCount] = EOF;

    return task;
}



int dispatchFileTasksRoundRobin(char* filename,int nextDispatch,int size, Result* result){
    FILE *file = fopen(filename, "rb");
    if (file == NULL){
        return nextDispatch;
    }


    Task task;
    
    while(true){
         //get chunk
        task = readBytes(file);
        //exit if no chunk
        if (task.byteCount<=0)
            break;

        //send size of next chunk, chunk
        MPI_Send(&(task.byteCount) , 1 , MPI_INT , nextDispatch , 0 , MPI_COMM_WORLD);    
        MPI_Send(task.bytes, size , MPI_CHAR , nextDispatch , 0 , MPI_COMM_WORLD);
        free(task.bytes);
        //advance dispatch number, wraps back to 1 after size
        nextDispatch++;
        if (nextDispatch>=size)
            nextDispatch=1;
    }

return nextDispatch;
}

void mergeChunks(int size, Result* results, int resultCount){
    int nextReceive = 1;
    int read; //move data here
    //for each file
    for(int i=0;i<resultCount;i++){
        Result res = results[i];
        //for each determinant
        for (int k=0;k<res.chunks;k++){
            //get wc,start vowel, end consonant
            MPI_Recv(&read,1,MPI_INT,nextReceive,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            res.wordCount+=read;
            MPI_Recv(&read,1,MPI_INT,nextReceive,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            res.vowelStartCount+=read;
            MPI_Recv(&read,1,MPI_INT,nextReceive,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            res.consonantEndCount+=read;
            //avance dispatch
            nextReceive++;
            if(nextReceive>=size)
                nextReceive=1;
        }
    }

}
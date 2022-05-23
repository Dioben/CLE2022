/**
 * @file worker.c (implementation file)
 *
 * @brief Problem name: multiprocess determinant calculation
 *
 * Contains implementation of the worker threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "worker.h"
#include "dispatcher.h"
#include "utfUtils.h"


/**
 * @brief Reads an UTF-8 character from a byte array.
 *
 * @param bytesRead where to start reading the character
 * @param bytes byte array
 * @return UTF-8 character
 */
int readLetterFromBytes(int *bytesRead, char *bytes)
{
    int letter = bytes[(*bytesRead)++] & 0x000000ff;

    // how many extra bytes need to be read after the first to get a full character
    // if -1 initial byte is either invalid or the end of the chunk
    int loops = -1 + byte0utf8(letter) + 2 * byte1utf8(letter) + 3 * byte2utf8(letter) + 4 * byte3utf8(letter);
    if (loops < 0)
    {
        // if not end of the chunk
        if (letter != 0x000000ff)
        {
            errno = EINVAL;
            perror("Invalid text found");
        }
        return EOF;
    }

    for (int i = 0; i < loops; i++)
    {
        letter <<= 8;
        letter += bytes[(*bytesRead)++] & 0x000000ff;
    }

    return letter;
}
/**
 * @brief Calculates the result from a task.
 *
 * @param task Task struct
 * @return Result struct
 */
static Result parseTask(int byteCount, char* bytes)
{
    Result result = {.vowelStartCount = 0,
                     .consonantEndCount = 0,
                     .wordCount = 0};
    int bytesRead = 0;

    // process all words in byte array
    while (bytesRead < byteCount)
    {
        int nextLetter = readLetterFromBytes(&bytesRead, bytes);
        int letter;

        // while not part of a word, read next character
        while (isSeparator(nextLetter) || isBridge(nextLetter))
        {
            nextLetter = readLetterFromBytes(&bytesRead, bytes);
        }

        if (nextLetter == EOF || bytesRead >= byteCount)
            break;

        result.wordCount++;

        if (isVowel(nextLetter))
            result.vowelStartCount += 1;

        // read characters until end of word
        do
        {
            letter = nextLetter;
            nextLetter = readLetterFromBytes(&bytesRead, bytes);
        } while (nextLetter != EOF && !isSeparator(nextLetter));

        if (isConsonant(letter))
            result.consonantEndCount += 1;

        if (nextLetter == EOF)
            break;
    }

    return result;
}

void whileTasksWorkAndSendResult()
{
    // task size, how much memory we've allocated, task itself,result of calculus
    int size;
    int currentMax = 0;
    char * chunk;
    Result result;
    
    MPI_Request req = MPI_REQUEST_NULL;
    while (true)
    {
        //receive next task chunk size
        MPI_Recv(&size,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        //signal to stop working
        if (size<1){
            MPI_Wait(&req,MPI_STATUS_IGNORE); //wait for last response to be read before shutdown
            break;
        }
        //free last request's handler if not in first loop
        if (req !=MPI_REQUEST_NULL)
            MPI_Request_free(&req);

        //our current chunk buffer isnt large enough
        if (size>currentMax){
            //matrix has not been allocated yet
            if (currentMax == 0){
                chunk = malloc(sizeof(char) *size);
            }//matrix has been allocated
            else{
                chunk = realloc(chunk,size);
            }
            currentMax = size;
        }
        //receive chunk
        MPI_Recv(chunk,size,MPI_CHAR,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        result = parseTask(size,chunk);
        //send back result, WC, startVowel , endConsonant
        MPI_Isend( &result.wordCount , 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD , &req);
        MPI_Request_free(&req);
        MPI_Isend( &result.vowelStartCount , 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD , &req);
        MPI_Request_free(&req);
        MPI_Isend( &result.consonantEndCount , 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD , &req);
    }

    if (currentMax>0)
        free(chunk);

}
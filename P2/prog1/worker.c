/**
 * @file worker.c (implementation file)
 *
 * @brief Problem name: multiprocess word count with multithreaded dispatcher
 *
 * Contains implementation of the worker processes.
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
#include "sharedRegion.h"

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
 * @param byteCount number of bytes in the task
 * @param bytes array with the bytes
 * @return Result struct
 */
static Result parseTask(int byteCount, char *bytes)
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

        if (nextLetter == EOF || bytesRead >= byteCount)
            break;

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

/**
 * @brief Worker process loop.
 *
 * Receives tasks via send and returns results from tasks.
 */
void whileTasksWorkAndSendResult()
{
    int chunkSize;      // chunk size, in bytes
    char *chunk;        // task
    int currentMax = 0; // how many bytes have been allocated for chunks
    Result result;      // result of the task processing
    int sendArray[3];

    MPI_Request req = MPI_REQUEST_NULL;
    while (true)
    {
        // receive next task chunk size
        MPI_Recv(&chunkSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // signal to stop working
        if (chunkSize < 1)
        {
            MPI_Wait(&req, MPI_STATUS_IGNORE); // wait for last response to be read before shutdown
            break;
        }

        // if our current chunk buffer isnt large enough
        if (chunkSize > currentMax)
        {
            // if matrix has not been allocated yet
            if (currentMax == 0)
                chunk = malloc(sizeof(char) * chunkSize);
            else
                chunk = realloc(chunk, sizeof(char) * chunkSize);

            currentMax = chunkSize;
        }
        // receive chunk
        MPI_Recv(chunk, chunkSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        result = parseTask(chunkSize, chunk);

        // wait for last send to cleared
        if (req != MPI_REQUEST_NULL)
            MPI_Wait(&req, MPI_STATUS_IGNORE);

        // send back result
        sendArray[0] = result.wordCount;
        sendArray[1] = result.vowelStartCount;
        sendArray[2] = result.consonantEndCount;
        MPI_Isend(sendArray, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
    }

    if (currentMax > 0)
        free(chunk);
}
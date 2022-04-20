/**
 * @file worker.c (implementation file)
 *
 * @brief Problem name: multithreaded word count
 *
 * Contains implementation of the worker threads.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "worker.h"
#include "sharedRegion.h"

/** @brief If byte is a start of an UTF-8 character and implies a 1 byte length character. */
#define byte0utf8(byte) !(byte >> 7)

/** @brief If byte is a start of an UTF-8 character and implies a 2 byte length character. */
#define byte1utf8(byte) (byte >= 192 && byte <= 223)

/** @brief If byte is a start of an UTF-8 character and implies a 3 byte length character. */
#define byte2utf8(byte) (byte >= 224 && byte <= 239)

/** @brief If byte is a start of an UTF-8 character and implies a 4 byte length character. */
#define byte3utf8(byte) (byte >= 240 && byte <= 247)

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
 * @brief Returns if UTF-8 character is a bridge character.
 *
 * @param letter UTF-8 character
 * @return if character is a bridge
 */
bool isBridge(int letter)
{
    return (
        letter == 0x27                              // '
        || letter == 0x60                           // `
        || 0xE28098 == letter || letter == 0xE28099 //  ’
    );
}
/**
 * @brief Returns if UTF-8 character is a vowel.
 *
 * @param letter UTF-8 character
 * @return if character is a vowel
 */
bool isVowel(int letter)
{
    return (
        letter == 0x41                            // A
        || letter == 0x45                         // E
        || letter == 0x49                         // I
        || letter == 0x4f                         // O
        || letter == 0x55                         // U
        || letter == 0x61                         // a
        || letter == 0x65                         // e
        || letter == 0x69                         // i
        || letter == 0x6f                         // o
        || letter == 0x75                         // u
        || (0xc380 <= letter && letter <= 0xc383) // À Á Â Ã
        || (0xc388 <= letter && letter <= 0xc38a) // È É Ê
        || (0xc38c == letter || letter == 0xc38d) // Ì Í
        || (0xc392 <= letter && letter <= 0xc395) // Ò Ó Ô Õ
        || (0xc399 == letter || letter == 0xc39a) // Ù Ú
        || (0xc3a0 <= letter && letter <= 0xc3a3) // à á â ã
        || (0xc3a8 <= letter && letter <= 0xc3aa) // è é ê
        || (0xc3ac == letter || letter == 0xc3ad) // ì í
        || (0xc3b2 <= letter && letter <= 0xc3b5) // ò ó ô õ
        || (0xc3b9 == letter || letter == 0xc3ba) // ù ú
    );
}
/**
 * @brief Returns if UTF-8 character is a consonant.
 *
 * @param letter UTF-8 character
 * @return if character is a consonant
 */
bool isConsonant(int letter)
{
    return (
        letter == 0xc387                      // Ç
        || letter == 0xc3a7                   // ç
        || (0x42 <= letter && letter <= 0x44) // B C D
        || (0x46 <= letter && letter <= 0x48) // F G H
        || (0x4a <= letter && letter <= 0x4e) // J K L M N
        || (0x50 <= letter && letter <= 0x54) // P Q R S T
        || (0x56 <= letter && letter <= 0x5a) // V W X Y Z
        || (0x62 <= letter && letter <= 0x64) // b c d
        || (0x66 <= letter && letter <= 0x68) // f g h
        || (0x6a <= letter && letter <= 0x6e) // j k l m n
        || (0x70 <= letter && letter <= 0x74) // p q r s t
        || (0x76 <= letter && letter <= 0x7a) // v w x y z
    );
}
/**
 * @brief Returns if UTF-8 character is a separator character.
 *
 * @param letter UTF-8 character
 * @return if character is a separator
 */
bool isSeparator(int letter)
{
    return (
        letter == 0x20                              // space
        || letter == 0x9                            // \t
        || letter == 0xA                            // \n
        || letter == 0xD                            // \r
        || letter == 0x5b                           // [
        || letter == 0x5d                           // ]
        || letter == 0x3f                           // ?
        || letter == 0xc2ab                         // «
        || letter == 0xc2bb                         // »
        || letter == 0xe280a6                       // …
        || 0x21 == letter || letter == 0x22         // ! "
        || 0x28 == letter || letter == 0x29         // ( )
        || (0x2c <= letter && letter <= 0x2e)       // , - .
        || 0x3a == letter || letter == 0x3b         // : ;
        || 0xe28093 == letter || letter == 0xe28094 // – —
        || 0xe2809c == letter || letter == 0xe2809d // “ ”
    );
}

/**
 * @brief Reads a chunk from a file stream into a byte array.
 *
 * @param file file stream to be read
 * @return Task struct with the number of bytes read and byte array
 */
static Task readBytes(FILE *file)
{
    Task task = {.fileIndex = -1,
                 .byteCount = -1,
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

/**
 * @brief Calculates the result from a task.
 *
 * @param task Task struct
 * @return Result struct
 */
static Result parseTask(Task task)
{
    Result result = {.vowelStartCount = 0,
                     .consonantEndCount = 0,
                     .wordCount = 0};
    int bytesRead = 0;

    // process all words in byte array
    while (bytesRead < task.byteCount)
    {
        int nextLetter = readLetterFromBytes(&bytesRead, task.bytes);
        int letter;

        // while not part of a word, read next character
        while (isSeparator(nextLetter) || isBridge(nextLetter))
        {
            nextLetter = readLetterFromBytes(&bytesRead, task.bytes);
        }

        if (nextLetter == EOF)
            break;

        result.wordCount++;

        if (isVowel(nextLetter))
            result.vowelStartCount += 1;

        // read characters until end of word
        do
        {
            letter = nextLetter;
            nextLetter = readLetterFromBytes(&bytesRead, task.bytes);
        } while (nextLetter != EOF && !isSeparator(nextLetter));

        if (isConsonant(letter))
            result.consonantEndCount += 1;

        if (nextLetter == EOF)
            break;
    }

    return result;
}

/**
 * @brief Uses a file to create tasks.
 *
 * @param fileIndex index of the file
 */
static void parseFile(int fileIndex)
{
    FILE *file = fopen(files[fileIndex], "rb");
    if (file == NULL)
        return;
    Task task;

    // while file has content create tasks
    while (true)
    {
        task = readBytes(file);

        // if no bytes were read it means we reached EOF
        if (task.byteCount == 0) {
            free(task.bytes);
            break;
        }

        task.fileIndex = fileIndex;

        // if FIFO is full process task instead
        if (!putTask(task))
        {
            updateResult(fileIndex, parseTask(task));
            free(task.bytes);
        }
    }
}

/**
 * @brief Worker thread.
 *
 * Its role is both to read files to generate tasks and to calculate results from tasks.
 *
 * @return pointer to the identification of this thread
 */
void *worker()
{
    int fileIndex;

    // get a file and create tasks
    while ((fileIndex = getNewFileIndex()) < totalFileCount)
    {
        if (fileIndex < 0)
        {
            if (fileIndex == -1)
            {
                errno = EDEADLK;
                perror("Error on getNewFileIndex() lock");
            }
            else
            {
                errno = EDEADLK;
                perror("Error on getNewFileIndex() unlock");
            }
            continue;
        }
        parseFile(fileIndex);
    }
    decreaseReaderCount();

    Task task;

    // while there are tasks process them
    while ((task = getTask()).fileIndex != -1)
    {
        updateResult(task.fileIndex, parseTask(task));
        free(task.bytes);
    }

    pthread_exit((int *)EXIT_SUCCESS);
}
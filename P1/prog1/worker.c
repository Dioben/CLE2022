#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "worker.h"
#include "sharedRegion.h"

#define byte0utf8(byte) !((byte & 0x000000ff) >> 7)
#define byte1utf8(byte) ((byte & 0x000000ff) >= 192 && (byte & 0x000000ff) <= 223)
#define byte2utf8(byte) ((byte & 0x000000ff) >= 224 && (byte & 0x000000ff) <= 239)
#define byte3utf8(byte) ((byte & 0x000000ff) >= 224 && (byte & 0x000000ff) <= 239)

static const int MAX_BYTES_READ = 500;
static const int BYTES_READ_BUFFER = 50;

int readLetterFromFile(FILE *file)
{
    int letter = 0;
    if (fread(&letter, 1, 1, file) != 1)
        return EOF;

    int loops = 0;
    if (byte0utf8(letter))
        return letter;
    else if (byte1utf8(letter))
        loops = 1;
    else if (byte2utf8(letter))
        loops = 2;
    else if (byte3utf8(letter))
        loops = 3;
    else
    {
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

int readLetterFromBytes(int *bytesRead, char *bytes)
{
    int letter = bytes[(*bytesRead)++] & 0x000000ff;

    int loops = 0;
    if (byte0utf8(letter))
        return letter;
    else if (byte1utf8(letter))
        loops = 1;
    else if (byte2utf8(letter))
        loops = 2;
    else if (byte3utf8(letter))
        loops = 3;
    else
    {
        // used to find end of bytes so this perror triggers a lot even though it isnt an error
        // perror("Invalid text or EOF found");
        return EOF;
    }
    for (int i = 0; i < loops; i++)
    {
        letter <<= 8;
        letter += bytes[(*bytesRead)++] & 0x000000ff;
    }
    return letter;
}

bool isBridge(int letter)
{
    return (
        letter == 0x27                              // '
        || letter == 0x60                           // `
        || 0xE28098 == letter || letter == 0xE28099 //  ’
    );
}

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

static Task readBytes(FILE *file)
{
    Task task = {.fileIndex = -1,
                 .byteCount = -1,
                 .bytes = malloc(sizeof(char) * MAX_BYTES_READ)};
    char bytes[MAX_BYTES_READ];
    task.byteCount = fread(&bytes, 1, (MAX_BYTES_READ - BYTES_READ_BUFFER), file);

    if (task.byteCount != MAX_BYTES_READ - BYTES_READ_BUFFER)
    {
        bytes[task.byteCount] = 0xff;
        memcpy(task.bytes, bytes, sizeof(char) * MAX_BYTES_READ);
        return task;
    }

    while (true)
    {
        if (byte0utf8(bytes[task.byteCount - 1]))
            break;
        else if (byte1utf8(bytes[task.byteCount - 1]))
            task.byteCount += fread(&bytes[task.byteCount], 1, 1, file);
        else if (byte2utf8(bytes[task.byteCount - 1]))
            task.byteCount += fread(&bytes[task.byteCount], 1, 2, file);
        else if (byte3utf8(bytes[task.byteCount - 1]))
            task.byteCount += fread(&bytes[task.byteCount], 1, 3, file);
        else
        {
            if (fread(&bytes[task.byteCount], 1, 1, file) != 1)
                break;
            task.byteCount++;
            continue;
        }
        break;
    }
    int letter;
    do
    {
        letter = readLetterFromFile(file);
        int loops = ((letter & 0xff000000) != 0) + ((letter & 0xffff0000) != 0) + ((letter & 0xffffff00) != 0);
        for (int i = loops; i >= 0; i--)
        {
            bytes[task.byteCount++] = letter >> (8 * i);
        }
    } while (!(isSeparator(letter) || isBridge(letter) || letter == EOF));

    bytes[task.byteCount] = 0xff;
    memcpy(task.bytes, bytes, sizeof(char) * MAX_BYTES_READ);

    return task;
}

static Result parseTask(Task task)
{
    Result result = {.vowelStartCount = 0,
                     .consonantEndCount = 0,
                     .wordCount = 0};
    int bytesRead = 0;
    while (bytesRead < task.byteCount)
    {
        int nextLetter = readLetterFromBytes(&bytesRead, task.bytes);
        int letter;

        while (isSeparator(nextLetter) || isBridge(nextLetter))
        {
            nextLetter = readLetterFromBytes(&bytesRead, task.bytes);
        }

        if (nextLetter == EOF)
            break;

        result.wordCount++;

        if (isVowel(nextLetter))
            result.vowelStartCount += 1;

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

static void parseFile(int fileIndex)
{
    FILE *file = fopen(files[fileIndex], "rb");
    if (file == NULL)
        return;
    bool isEOF = false;
    Task task;
    while (!isEOF)
    {
        task = readBytes(file);
        if (task.byteCount < MAX_BYTES_READ - BYTES_READ_BUFFER)
            isEOF = true;
        task.fileIndex = fileIndex;
        if (!putTask(task))
        {
            updateResult(fileIndex, parseTask(task));
            free(task.bytes);
        }
    }
}

void *worker(void *par)
{
    int fileIndex;
    while ((fileIndex = getNewFileIndex()) < totalFileCount)
    {
        if (fileIndex < 0)
        {
            if (fileIndex == -1)
                perror("Error on getNewFileIndex() lock");
            else
                perror("Error on getNewFileIndex() unlock");
            continue;
        }
        parseFile(fileIndex);
    }
    decreaseReaderCount();
    Task task;
    while ((task = getTask()).fileIndex != -1)
    {
        updateResult(task.fileIndex, parseTask(task));
        free(task.bytes);
    }

    pthread_exit((int *)EXIT_SUCCESS);
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>

/* allusion to internal functions */
static void printUsage(char *cmdName);

struct fileStats
{
    unsigned long words;
    unsigned long startsVowel;
    unsigned long endsConsonant;
};

/*
receives the file pointer
reads x bytes based on utf 8 prefixes
all "word-ending" chars are returned as \0 instead and we skip "word-bridgers" -> bridge with recursion?
write down the masks at some point
*/
int readLetter(FILE *file)
{
    unsigned int readspot = 0;
    size_t readBytes = fread(&readspot, 1, 1, file);
    if (readBytes != 1)
        return EOF;
    int loops = 0;
    if (!(readspot >> 7)) // leftmost byte is 0
        return readspot;
    else if (readspot <= 223 && readspot >= 192)
    { // leftmost 2 bytes are 1, followed by 0 -> 128 and 64 are in, 32 is not
        loops = 1;
    }
    else if (readspot <= 239 && readspot >= 224)
    { // leftmost 3 bytes are 1, followed by 0 -> 16 is not in, 224 min
        loops = 2;
    }
    else if (readspot <= 247 && readspot >= 240)
    { // leftmost 4 bytes are 1, followed by 0 -> 8 is not in and there's a 240 minimum
        loops = 3;
    }
    else
    {
        perror("Invalid text found");
        return EOF;
    }
    for (int i = 0; i < loops; i++)
    {
        readspot <<= 8;
        fread(&readspot, 1, 1, file);
    }

    return readspot;
}

/*
reads letter to check if it is a bridge
returns 1 if it is a bridge
returns 0 if not
*/
int isBridge(unsigned int letter)
{
    return (
        letter == 0x27                              // '
        || letter == 0x60                           // `
        || 0xE28098 == letter || letter == 0xE28099 // ‘ ’
    );
}

/*
reads letter to check if it is a vowel
returns 1 if it is a vowel
returns 0 if not
*/
int isVowel(unsigned int letter)
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

/*
reads letter to check if it is a consonant
returns 1 if it is a consonant
returns 0 if not
*/
int isConsonant(unsigned int letter)
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

/*
reads letter to check if it is a separator (separates 2 words)
returns 1 if it is a separator
returns 0 if not
*/
int isSeparator(unsigned int letter)
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

/*
read chars with letter function until \0 or EOF
return a 4 bit value
4-bit: there was no word to read
3-bit: is EOF
2-bit: ends with consonant
1-bit: begins with vowel
*/
char readWord(FILE *file)
{
    char retval = 0;
    unsigned int nextLetter = readLetter(file);
    unsigned int letter;
    while (isSeparator(nextLetter) || isBridge(nextLetter))
    {
        nextLetter = readLetter(file);
    }

    if (nextLetter == EOF)
        return 8; // 1000

    if (isVowel(nextLetter))
        retval += 1; // 0001

    do
    {
        letter = nextLetter;
        nextLetter = readLetter(file);
    } while (nextLetter != EOF && !isSeparator(nextLetter));

    if (isConsonant(letter))
        retval += 2; // 0010

    if (nextLetter == EOF)
        retval += 4; // 0100

    return retval;
}

/*
Given a file name, returns a struct containing:
total word count
words starting with vowel
words ending in consonant
*/
struct fileStats parseFile(char *fileName)
{
    struct fileStats stats;
    stats.words = 0;
    stats.startsVowel = 0;
    stats.endsConsonant = 0;
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
        return stats;
    char wordStats;
    do
    {
        wordStats = readWord(file);
        stats.words += !(wordStats >> 3) & 1; // 8-bit is not active
        stats.startsVowel += wordStats & 1;
        stats.endsConsonant += (wordStats & 2) >> 1;
    } while (!(wordStats >> 2));
    fclose(file);
    return stats;
}

int main(int argc, char **args)
{
    int opt;    /* selected option */
    opterr = 0; // this seems to set error throwing to manual (getopt can now return ?)
    unsigned int filestart = -1;
    unsigned int filespan = 0;

    do
    {
        switch ((opt = getopt(argc, args, "f:h")))
        {
        case 'f': /* file name */
            filestart = optind - 1;
            for (filespan = 0; filestart + filespan < argc && args[filespan + filestart][0] != '-'; filespan++)
            {
                // constantly checks if within bounds and isnt next OPT
                // this loop only serves to advance filespan
            }
            break;
        case 'h': /* help mode */
            printUsage(basename(args[0]));
            return EXIT_SUCCESS;
        case '?': /* invalid option */
            fprintf(stderr, "%s: invalid option\n", basename(args[0]));
            printUsage(basename(args[0]));
            return EXIT_FAILURE;
        case -1:
            break;
        }
    } while (opt != -1);

    if (argc == 1) // no args
    {
        fprintf(stderr, "%s: invalid format\n", basename(args[0]));
        printUsage(basename(args[0]));
        return EXIT_FAILURE;
    }
    if (filestart == -1 || filespan == 0) // no files
    {
        fprintf(stderr, "%s: file name is missing\n", basename(args[0]));
        printUsage(basename(args[0]));
        return EXIT_FAILURE;
    }

    double t0, t1, t2; /* time limits */
    t2 = 0.0;

    char *file;

    printf("%-30s %15s %15s %15s\n", "File Name", "Word Count", "Starts Vowel", "Ends Consonant");
    for (int i = 0; i < filespan; i++)
    {
        file = args[filestart + i];

        t0 = ((double)clock()) / CLOCKS_PER_SEC;
        struct fileStats stats = parseFile(file);
        t1 = ((double)clock()) / CLOCKS_PER_SEC;
        t2 += t1 - t0;
        printf("%-30s %15lu %15lu %15lu\n", file, stats.words, stats.startsVowel, stats.endsConsonant);
    }
    printf("\nElapsed time = %.6f s\n", t2);
    return 0;
}

/**
 *  \brief Print command usage.
 *
 *  A message specifying how the program should be called is printed.
 *
 *  \param cmdName string with the name of the command
 */
static void printUsage(char *cmdName)
{
    fprintf(stderr, "\nSynopsis: %s OPTIONS [filename / positive number]\n"
                    "  OPTIONS:\n"
                    "  -h      --- print this help\n"
                    "  -f      --- filenames, space separated\n",
            cmdName);
}
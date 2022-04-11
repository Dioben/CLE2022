/* ‘ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test1()
{
    char *fileName = "test.c";
    FILE *file = fopen(fileName, "rb");
    int null;
    char letter[4] = {0x00, 0x00, 0x00, 0x00};
    fread(&null, 1, 3, file);
    fread(&letter, 1, 3, file);
    int intLetter =
        (letter[0] << 24) + ((letter[1] << 16) & 0x00ff0000) + ((letter[2] << 8) & 0x0000ff00) + (letter[3] & 0x000000ff);
    printf("%x %x %x %x\n", letter[0], letter[1], letter[2], letter[3]);
    printf("%x\n", intLetter);
    char letter2[4];
    letter2[0] = intLetter >> 24;
    letter2[1] = intLetter >> 16;
    letter2[2] = intLetter >> 8;
    letter2[3] = intLetter;
    printf("%s\n", letter2);
}

void test2()
{
    char *fileName = "test.c";
    FILE *file = fopen(fileName, "rb");
    int null;
    char letter[4] = {0x00, 0x00, 0x00, 0x00};
    fread(&null, 1, 3, file);
    size_t readSize = fread(&letter, 1, 1, file);
    printf("%ld\n", readSize);
    fread(&letter[readSize], 1, 2, file);
    printf("%x %x %x %x\n", letter[0], letter[1], letter[2], letter[3]);
}

void test3()
{
    char *fileName = "test.c";
    FILE *file = fopen(fileName, "rb");
    int null;
    char letter[4];
    fread(&null, 1, 1, file);
    fread(&letter, 1, 4, file);
    printf("%x\n", null);
    printf("%x %x %d %x\n", letter[0] & 0x000000ff, letter[1] & 0x000000ff, letter[2] & 0x000000ff, letter[3] & 0x000000ff);
    printf("%x\n", letter[0] >> 7);
}

int main()
{
    test3();
}
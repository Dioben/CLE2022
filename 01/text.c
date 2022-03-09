#include <stdio.h>


struct fileStats{
    unsigned long words;
    unsigned long startsVowel;
    unsigned long endsConsonant;
};

int readLetter(FILE * file){
    /*
    receives the file pointer
    reads x bytes based on utf 8 prefixes
    all "word-ending" chars are returned as \0 instead and we skip "word-bridgers" -> bridge with recursion?
    write down the masks at some point
    */
   unsigned int readspot;
   fread(&readspot,1,1,file);
   printf("read %u\n",readspot);

   return 0;
}

char readWord(FILE * file){
    /*
    read chars with letter function until \0
    return a 3 bit value
    3-bit = is EOF
    2-bit : ends with consonant
    1-bit: begins with vowel
    */
   
   return 7;
}

/*
Given a file name, returns a struct containing:
total word count
words starting with vowel
words ending in consonant
*/
struct fileStats parseFile(char * fileName){
    struct fileStats stats;
    stats.words = 0;
    stats.startsVowel = 0;
    stats.endsConsonant = 0;
    FILE * file = fopen(fileName,"rb");
    if (file==NULL)
        return stats;
    char wordStats;
    do{
        wordStats = readWord(file);
        stats.words++;
        stats.startsVowel+= wordStats & 1;
        stats.endsConsonant+= (wordStats & 2) >> 1;
    }
   while (! (wordStats>>2) );
   fclose(file);
   return stats;
}





int main(int argc , char** args){
    printf("%-30s %15s %15s %15s\n", "File Name", "Word Count", "Starts Vowel", "Ends Consonant");
    for(int i=1;i<argc;i++){
        struct fileStats stats = parseFile(args[i]);
        printf("%-30s %15lu %15lu %15lu\n", args[i], stats.words, stats.startsVowel, stats.endsConsonant);
    }
    
    return 0;
}
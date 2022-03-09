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
   unsigned int readspot =0;
   fread(&readspot,1,1,file);
   if (! (readspot >> 7)) //leftmost byte must be 0
        return readspot;
    else if(readspot<=223 && readspot>= 192){ //leftmost 2 bytes are 1, followed by 0 -> 128 and 64 are in, 32 is not
        readspot<<=8;
        fread(&readspot+1,1,1,file); //TODO: I have NO CLUE whether this works
    }
    else if (readspot<=239 && readspot>= 224){//leftmost 3 bytes are 1, followed by 0 -> 16 is not in, 224 min
        readspot<<=16;
        fread(&readspot+1,2,1,file);
    } //TODO: I have NO CLUE whether this works
    else if (readspot<=247 && readspot>= 240){//leftmost 4 bytes are 1, followed by 0 -> 8 is not in and there's a 240 minimum
        readspot<<=24;
        fread(&readspot+1,3,1,file); //TODO: I have NO CLUE whether this works
    } 
    else{
        perror("Invalid text found");
        return EOF;
    }

   return readspot;
}

char readWord(FILE * file){
    /*
    read chars with letter function until \0 or EOF
    return a 3 bit value
    3-bit = is EOF
    2-bit : ends with consonant
    1-bit: begins with vowel
    */
   char retval = 0;
   unsigned int nextLetter = readLetter(file);
   unsigned int letter;
   //TODO: scrutinize first word
   do{
        letter = nextLetter;
        nextLetter = readLetter(file);
   }while(1);//TODO: figure out condition
    //TODO: Scrutinize last char

   return retval;
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
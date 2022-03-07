#include <stdio.h>

int parseFile(char * fileName){
    /*
    just loop readWord a bunch until EOF
    for every call word count +1, increment other values with bit mask
    return the 3 int struct to main, it'll print it
    */
   long words = 0;
   long startsVowel = 0;
   long endsConsonant = 0;

   while (!EOF){ //TODO: RESEARCH ACTUAL EOF
       wordStats = readWord(fileReader); //TODO: DECLARE AN ACTUAL READER
       startsVowel+= wordStats & 1;
       endsConsonant+= wordStats >> 1;
   }
}

int readWord(FILE * file){
    /*
    read chars with letter function until \0
    return a 2bit value
    2-bit : ends with consonant
    1-bit: begins with vowel
    */
}

int readLetter(FILE * file){
    /*
    receives the file pointer
    reads x bytes based on utf 8 prefixes
    returns the parsed ASCII char
    all "word-ending" chars are returned as \0 instead and we skip "word-bridgers" -> bridge with recursion?
    write down the masks at some point
    */
}

int main(int argc , char** args){
    for(int i=1;i<argc;i++){
        char * pName = args[i];
        printf("Hello %s\n",pName);
    }
    
    return 0;
}

//main -> file func -> read word func (return stats) -> read char func (autoparse based on rules)
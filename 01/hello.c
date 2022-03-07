#include <stdio.h>

int main(int argc , char** args){
    for(int i=1;i<argc;i++){
        char * pName = args[i];
        printf("Hello %s\n",pName);
    }
    
    return 0;
}

//main -> file func -> read word func (return stats) -> read char func (autoparse based on rules)
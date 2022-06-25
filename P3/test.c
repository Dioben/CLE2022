#include <stdio.h>

int main() {
   FILE *fp;

   fp = fopen("mattest.bin", "w");
   int count = 1;
   int size = 4;
   double matrix[] = {
    0,4,4,4,
    1,1,1,1,
    3,3,7,3,
    3,2,2,-2
   };
   fwrite(&count,4,1,fp);
   fwrite(&size,4,1,fp);
   fwrite(matrix,8,size*size,fp);
   fclose(fp);
}
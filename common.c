#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


int getbit16(char *bitstring, int bitnr){
  char bitchar;
  int lendian;
  int result;

  if (bitnr > 15) {
    printf("Requesting bit > 15!!\n");
    exit(0);
  }

  lendian = 15 -  bitnr;

  bitchar = bitstring[lendian];
  result = atoi(&bitchar);
  return result;
}

int getbit3(char *bitstring, int bitnr){
  char bitchar;
  int result;

  if (bitnr > 3) {
    printf("Requesting bit > 3!!\n");
    exit(0);
  }


  bitchar = bitstring[bitnr];
  result = atoi(&bitchar);
  return result;
}

int bin2_to_dec(char *bin) {
  int result;

  result=0;
  if (bin[0] == '1')
    result += 2;
  if (bin[1] == '1')
    result += 1;
  return result;
}


int bin3_to_dec(char *bin) {
  int result;

  result=0;
  if (bin[0] == '1')
    result += 4;
  if (bin[1] == '1')
    result += 2;
  if (bin[2] == '1')
    result += 1;
  return result;
}
int bin7_to_dec(char *bin) {
  int result;

  result=0;
  if (bin[0] == '1')
    result += 64;
  if (bin[1] == '1')
    result += 32;
  if (bin[2] == '1')
    result += 16;
  if (bin[3] == '1')
    result += 8;
  if (bin[4] == '1')
    result += 4;
  if (bin[5] == '1')
    result += 2;
  if (bin[6] == '1')
    result += 1;
  return result;
}
int bin10_to_dec(char *bin) {
  int result;

  result=0;
  if (bin[0] == '1')
    result += 512;
  if (bin[1] == '1')
    result += 256;
  if (bin[2] == '1')
    result += 128;
  if (bin[3] == '1')
    result += 64;
  if (bin[4] == '1')
    result += 32;
  if (bin[5] == '1')
    result += 16;
  if (bin[6] == '1')
    result += 8;
  if (bin[7] == '1')
    result += 4;
  if (bin[8] == '1')
    result += 2;
  if (bin[9] == '1')
    result += 1;
  return result;
}

int bin13_to_dec(char *bin) {
  int result;

  result=0;
  if (bin[0] == '1')
    result += 4196;
  if (bin[1] == '1')
    result += 2048;
  if (bin[2] == '1')
    result += 1024;
  if (bin[3] == '1')
    result += 512;
  if (bin[4] == '1')
    result += 256;
  if (bin[5] == '1')
    result += 128;
  if (bin[6] == '1')
    result += 64;
  if (bin[7] == '1')
    result += 32;
  if (bin[8] == '1')
    result += 16;
  if (bin[9] == '1')
     result += 8;
  if (bin[10] == '1')
     result += 4;
  if (bin[11] == '1')
     result += 2;
  if (bin[12] == '1')
     result += 1;

  return result;
}



char *decimal_to_binary16(int n) {
   int c, d, count;
   char *pointer;

   count = 0;
   pointer = (char*)malloc(16+1);

   if ( pointer == NULL )
      exit(EXIT_FAILURE);

   for ( c = 15 ; c >= 0 ; c-- )
   {
      d = n >> c;

      if ( d & 1 )
         *(pointer+count) = 1 + '0';
      else
         *(pointer+count) = 0 + '0';

      count++;
   }
   *(pointer+count) = '\0';

   return pointer;
}


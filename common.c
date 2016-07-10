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

int getbit6(char *bitstring, int bitnr){
  char bitchar;
  int result;

  if (bitnr > 6) {
    printf("Requesting bit > 15!!\n");
    exit(0);
  }


  bitchar = bitstring[bitnr];
  result = atoi(&bitchar);
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


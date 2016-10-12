/****************************************************************************
 *                Texas Instruments TMS320C10 DSP Disassembler              *
 *                                                                          *
 *                      Copyright (C) 1999 by Quench                        *
 *       You are not allowed to distribute this software commercially.      *
 *                      Written for the MAME project.                       *
 *                                                                          *
 *     Note: Data is expected to be read from source file as MSB first.     *
 *           This is a word based microcontroller.                          *
 *                                                                          *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "32010dsm.c"

int main(int argc,char *argv[])
{
  FILE *F;
  byte *Buf;
  long Counter=0;
  int  len=0, lentodump=0, offset=0, disasm_bytes=0;
  int  filelength=0, bytesread;
  char S[87];

  if(argc<2)
  {
      puts("TMS32010 Disassembler 1.0 by Quench (C)1998");
      puts("Note : Data is expected to be read from source file as MSB first.");
      puts("Usage: dis32010 <input file> [ <start-addr> [ <num-of-opcodes> ] ]");
      exit(0);
  }

  if(!(F=fopen(argv[1],"rb"))) {
      printf("\n%s: Can't open file %s\n",argv[0],argv[1]);exit(1);
  }
  argv++; argc--;
  if (argv[1]) {
      offset = strtol(argv[1],NULL,0);
      argv++; argc--;
  }
  if (argv[1]) {
      len = strtol(argv[1],NULL,0);
      argv++; argc--;
  }
  len*=2;
  filelength = fseek(F,0, SEEK_END);
  filelength = ftell(F);
  if ((len > (filelength - (offset*2))) || (len == 0)) len = filelength - (offset*2);
  lentodump = len;
  printf("Starting from %d, dumping %d opcodes (word size)\n",offset,(len/2));
  Buf=calloc((len+3),sizeof(char));
  if (Buf==NULL){
      printf("Out of Memory !!!");
      fclose(F);
      exit(1);
  }

  if (fseek(F,offset*2,0) != 0) {
      fprintf(stderr,"Error seeking to offset %d\n",offset);
      free(Buf);
      fclose(F);
      exit(1);
  }
  Counter = offset;
  bytesread = fread(Buf,1,len+2,F);
  if (bytesread >= len) {
      for (; len > 0; len -= disasm_bytes) {
         int ii;
         disasm_bytes = Dasm32010(S,Buf);
         printf("%04lX: ", Counter);
         Counter+=disasm_bytes;
         disasm_bytes *= 2;
         for (ii = 0; ii < disasm_bytes; ii+=2) {
             printf("%02.2x%02.2x ",Buf[ii],Buf[ii+1]);
         }
         for (; ii < 4; ii++) { printf("   "); }
         printf("\t%s\n",S);
         Buf += disasm_bytes;
      }
  }
  else {
        printf("ERROR length to dump was %d ", lentodump/2);
        printf(", but bytes read from file were %d\n", bytesread/2);
        free(Buf);fclose(F);exit(1);
  }
  free(Buf);fclose(F);return(0);
}

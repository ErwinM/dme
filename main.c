//C hello world example
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "units.h"
#include "types.h"



ushort ALU(ushort x, ushort y, char *func);
ushort readrom(ushort addr);
ushort readram(ushort addr);
void writeram(ushort addr, ushort value);
void signame_pass(ushort *clk);
void latch_pass(enum signalstate clk_phase);
int update_csig(enum control_sigs signame, enum signalstate state);
int update_bsig(int signame, ushort *value);

void init();

// Global variables
struct Areg *Areg;
struct Dreg *Dreg;
struct mux *ymux;
struct mux *imux;
ROM rom;
ROM *rom_p;
RAM ram;
RAM *ram_p;

enum signalstate csig[8];
ushort bsig[7];

struct clk clk;

char *ALUfunc[7];

int updated;

int main(int argc,char *argv[])
{
  int cycle;
  int tempie;
  init();

  bsig[ALUOUT]=66;
  printf("test: %s(%d)\n", BSIG_STRING[ALUOUT], ALUOUT);
  printf("value-1: %d", bsig[3]);
  printf("value: %d", bsig[ALUOUT]);
  printf("value+1: %d", bsig[5]);
  printf("--------------------------");
  tempie = 77;
  update_bsig(ALUOUT, &tempie);

  for(clk.tick = 0; clk.tick < 16; clk.tick++) {
    for(clk.phase=0; clk.phase<5; clk.phase++){

      printf("------------------------------------------------------\n");
      printf("cycle: %d.%d - PC: %x, A: %x, D: %x\n", clk.tick, clk.phase, bsig[PC], bsig[AREGOUT], bsig[DREGOUT]);
      printf("ALU: %x, zr: %x, ng: %x, ", bsig[ALUOUT], csig[ZR], csig[NG]);

      // signame phase - also reads ROM/RAM
      updated = 1;
      cycle = 0;
      while(updated==1 && cycle < 5) {
        updated = 0;
        cycle += 1;
        signame_pass(0);
      }
      printf("Stable after %d cycles.\n", cycle);

      // Latch pass - single pass!
      latch_pass(clk.phase);
    }
  }
}

void init(void) {

  //csig = malloc(sizeof(struct control_sig));
  //alusig = malloc(sizeof(struct ALU_sig));
  //bussig = calloc(1, sizeof(struct bus_sig));
  bsig[PC] = 0;

  /* Add.hack
  rom[0]=0x0002;
  rom[1]=0xec10;
  rom[2]=0x0003;
  rom[3]=0xe090;
  rom[4]=0x0000;
  rom[5]=0xe308;
  */

  rom[0]=0x0000;
  rom[1]=0xfc10;
  rom[2]=0x0001;
  rom[3]=0xf4d0;
  rom[4]=0x000a;
  rom[5]=0xe301;
  rom[6]=0x0001;
  rom[7]=0xfc10;
  rom[8]=0x000c;
  rom[9]=0xea87;
  rom[10]=0x0000;
  rom[11]=0xfc10;
  rom[12]=0x0002;
  rom[13]=0xe308;
  rom[14]=0x000e;
  rom[15]=0xea87;

  ram[0]=6;
  ram[1]=3;

  printf("Init done..\n");
  return;
}

void signame_pass(ushort *clk) {
  // fetch the instruction from ROM
  ushort instr;
  ushort mdr;
  char *instr_b;
  int jcond;
  char *jcond_s[4];
  int jcondmet;


  // Memory read
  instr = readrom(bsig[PC]);

  instr_b = decimal_to_binary16(instr);
  printf("instr: %s\n", instr_b);
  //printf("bit number 12: %d", getbit(instr_b, 15));
  printf("csig: ");

  // c1 - selain
  if (getbit16(instr_b, 15) == 0) {
    update_csig(SELAIN, HI);
  } else {
    update_csig(SELAIN, LO);
  }
  // c2 - loada - either A instruction or A is dest
  if (getbit16(instr_b, 5) == 1 || getbit16(instr_b, 15) == 0) {
    update_csig(LOADA, HI);
  } else {
    update_csig(LOADA, LO);
  }
  // c3 - loadd
  if (getbit16(instr_b, 4) == 1 && getbit16(instr_b, 15) == 1) {
    update_csig(LOADD, HI);
  } else {
    update_csig(LOADD, LO);
  }
  // c4 - sely
  if (getbit16(instr_b, 12) == 1) {
    update_csig(SELY, HI);
  } else {
    update_csig(SELY, LO);
  }
  // c7 - wren
  if (getbit16(instr_b, 3) == 1 && getbit16(instr_b, 15) == 1) {
    update_csig(WREN, HI);
  } else {
    update_csig(WREN, LO);
  }
  // c6 - jump
  memcpy(jcond_s, instr_b+13, 3);
  //printf("jcond_s is: %s\n", jcond_s);
  jcond = bin3_to_dec(jcond_s);
  //printf("jcond is: %d\n", jcond);

  jcondmet = 0;
  switch(jcond){
    case 1:
      if (csig[ZR] == LO && csig[NG]== LO)
        jcondmet = 1;
      break;
    case 2:
      if (csig[ZR] == HI)
        jcondmet = 1;
      break;
    case 3:
      if (csig[NG] == LO)
        jcondmet = 1;
      break;
    case 4:
      if (csig[NG] == HI)
        jcondmet = 1;
      break;
    case 5:
      if (csig[ZR] == LO)
        jcondmet = 1;
      break;
    case 6:
      if (csig[ZR] == HI || csig[NG] == HI)
        jcondmet = 1;
      break;
    case 7:
      jcondmet = 1;
      break;
  }
  if (getbit16(instr_b, 15) == 1 && jcondmet == 1) {
    // c instruction and jump condition is met
    update_csig(JUMP, HI);
    //printf("Jump is HI!\n");
  } else {
    update_csig(JUMP, LO);
  }

  printf("\nbus: ");

  // RAM
  mdr = readram(bsig[AREGOUT]);
  printf("MDR: %x ", mdr);
  update_bsig(INM, &mdr);

  // immMux
  if (csig[SELAIN] == HI) {
    update_bsig(AIN, &instr);
  } else {
    update_bsig(AIN, &bsig[ALUOUT]);
  }
  // YMux
  if (csig[SELY] == HI) {
    update_bsig(ALUY, &bsig[INM]);
  } else {
    update_bsig(ALUY, &bsig[AREGOUT]);
  }

  // ALU
  memcpy(ALUfunc, instr_b+4, 6);
  ALUfunc[6]="\0";
  //printf("ALU: %s", ALUfunc);
  ushort ALUtemp;
  ALUtemp = ALU(bsig[DREGOUT], bsig[ALUY], ALUfunc);
  update_bsig(ALUOUT, &ALUtemp);
}

void latch_pass(enum signalstate clk_phase){

  // only latch on rising edge
  if (clk_phase != clk_RE)
    return;

  printf("Latch pass\n");

  // Areg
  if (csig[LOADA] == HI) {
    bsig[AREGOUT] = bsig[AIN];
    printf("A <- %x\n", bsig[AIN]);
  }
  if (csig[LOADD] == HI) {
    bsig[DREGOUT] = bsig[ALUOUT];
    printf("D <- %x\n", bsig[ALUOUT]);
  }

  // RAM
  if (csig[WREN] == HI) {
    writeram(bsig[AREGOUT], bsig[ALUOUT]);
    printf("RAM[%x] <- %x\n", bsig[AREGOUT], bsig[AIN]);
  }

  // PC
  if (csig[JUMP] == HI) {
    bsig[PC] = bsig[AREGOUT];
  } else {
    bsig[PC] += 1;
  }
  printf("PC <- %x\n", bsig[PC]);
}


int update_csig(enum control_sigs signame, enum signalstate state) {
  if (csig[signame] != state) {
    updated = 1;
    printf("%s ", CSIG_STRING[signame]);
  }
  csig[signame] = state;
}

int update_bsig(int signame, ushort *value) {
  //printf("signame: %d", signame);
  if (bsig[signame] != *value) {
    updated = 1;
    printf("%s ", BSIG_STRING[signame]);
    //if (signame == AIN)
      //printf("v: %x", *value);
  }
  bsig[signame] = *value;
}

ushort ALU(ushort x, ushort y, char *func) {
  ushort result;

  if (getbit6(func, 0)==1)
    x = 0;
  if (getbit6(func, 1)==1)
    x = ~x;
  if (getbit6(func, 2)==1)
    y = 0;
  if (getbit6(func, 3)==1)
    y = ~y;
  if (getbit6(func, 4)==1) {
    result = x + y;
  } else {
    result = x & y;
  }
  if (getbit6(func, 5)==1) {
    result = ~result;
  }
  if (result == 0) {
    csig[ZR] = HI;
  } else {
    csig[ZR] = LO;
  }
  if (result < 0) {
    csig[NG] = HI;
  } else {
    csig[NG] = LO;
  }
  /*
  printf("Alu x: %d, ", x);
  printf("Alu y: %d, ", y);
  printf("Alu out: %d ", result);
  printf("func: %s\n", func);
  */
  return result;
}

ushort readrom(ushort addr) {
  //printf("Reading ROM at address: %x\n", addr);
  return rom[addr];
}

ushort readram(ushort addr) {
  //printf("Reading RAM at address: %x\n", addr);
  return ram[addr];
}

void writeram(ushort addr, ushort value) {
  ram[addr] = value;
}

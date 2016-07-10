//C hello world example
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "units.h"
#include "types.h"



void ALU(ushort *x, ushort *y, char *func, ushort *ALUout);
ushort readrom(ushort addr);
ushort readram(ushort addr);
void writeram(ushort addr, ushort value);
void signal_pass(ushort *clk);
void latch_pass(enum signalstate clk_phase);
int update_csig(enum signalstate *csignal, enum signalstate state);
int update_bsig(ushort *bsignal, ushort *value);

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
struct control_sig *csig;
struct ALU_sig *alusig;
struct bus_sig *bussig;
struct clk clk;

char *ALUfunc[7];

int updated;

int main()
{
  int cycle;
  int pcycle;
  init();

  for(clk.tick = 0; clk.tick < 6; clk.tick++) {
    for(clk.phase=0; clk.phase<5; clk.phase++){

      printf("------------------------------------------------------\n");
      printf("cycle: %d.%d - PC: %x, A: %x, D: %x\n", clk.tick, clk.phase, bussig->pc, bussig->Aregout, bussig->Dregout);

      // Signal phase - also reads ROM/RAM
      updated = 1;
      cycle = 0;
      while(updated==1 && cycle < 5) {
        updated = 0;
        cycle += 1;
        signal_pass(0);
      }
      printf("Stable after %d cycles.\n", cycle);

      // Latch pass - single pass!
      latch_pass(clk.phase);
    }
  }
}

void init(void) {


  Areg = (struct Areg *)calloc(1, sizeof(struct Areg));
  Dreg = (struct Dreg *)calloc(1, sizeof(struct Dreg));
  csig = malloc(sizeof(struct control_sig));
  alusig = malloc(sizeof(struct ALU_sig));
  bussig = calloc(1, sizeof(struct bus_sig));
  bussig->pc = 0;

  rom[0]=0x0002;
  rom[1]=0xec10;
  rom[2]=0x0003;
  rom[3]=0xe090;
  rom[4]=0x0000;
  rom[5]=0xe308;

  printf("Init done..\n");
  return;
}

void signal_pass(ushort *clk) {
  // fetch the instruction from ROM
  ushort instr;
  char *instr_b;
  int jcond;
  char *jcond_s[4];
  int jcondmet;


  // Memory read
  instr = readrom(bussig->pc);
  bussig->inM = readram(bussig->Aregout);

  instr_b = decimal_to_binary16(instr);
  printf("Instr code: %s\n", instr_b);
  //printf("bit number 12: %d", getbit(instr_b, 15));

  // c1 - selain
  if (getbit16(instr_b, 15) == 0) {
    update_csig(&csig->selain, HI);
  } else {
    update_csig(&csig->selain, LO);
  }
  // c2 - loada - either A instruction or A is dest
  if (getbit16(instr_b, 5) == 1 || getbit16(instr_b, 15) == 0) {
    update_csig(&csig->loada, HI);
  } else {
    update_csig(&csig->loada, LO);
  }
  // c3 - loadd
  if (getbit16(instr_b, 4) == 1) {
    update_csig(&csig->loadd, HI);
  } else {
    update_csig(&csig->loadd, LO);
  }
  // c4 - sely
  if (getbit16(instr_b, 12) == 1) {
    update_csig(&csig->sely, HI);
  } else {
    update_csig(&csig->sely, LO);
  }
  // c7 - wren
  if (getbit16(instr_b, 3) == 1) {
    update_csig(&csig->wren, HI);
  } else {
    update_csig(&csig->wren, LO);
  }
  // c6 - jump
  memcpy(jcond_s, instr_b+13, 3);
  //printf("jcond_s is: %s\n", jcond_s);
  jcond = bin3_to_dec(jcond_s);
  printf("jcond is: %d\n", jcond);

  jcondmet = 0;
  switch(jcond){
    case 1:
      if (alusig->zr == LO && alusig->ng == LO)
        jcondmet = 1;
      break;
    case 2:
      if (alusig->zr == HI)
        jcondmet = 1;
      break;
    case 3:
      if (alusig->ng == LO)
        jcondmet = 1;
      break;
    case 4:
      if (alusig->ng == HI)
        jcondmet = 1;
      break;
    case 5:
      if (alusig->zr == LO)
        jcondmet = 1;
      break;
    case 6:
      if (alusig->zr == HI || alusig->ng == HI)
        jcondmet = 1;
      break;
    case 7:
      jcondmet = 1;
      break;
  }
  if (getbit16(instr_b, 15) == 1 && jcondmet == 1) {
    // c instruction and jump condition is met
    update_csig(&csig->jump, HI);
    printf("Jump is HI!\n");
  } else {
    update_csig(&csig->jump, LO);
  }

  // immMux
  if (csig->selain == HI) {
    update_bsig(&bussig->ain, &instr);
  } else {
    update_bsig(&bussig->ain, &bussig->ALUout);
  }
  // YMux
  if (csig->sely == HI) {
    update_bsig(&bussig->ALUy, &bussig->inM);
  } else {
    update_bsig(&bussig->ALUy, &bussig->Aregout);
  }

  // ALU
  memcpy(ALUfunc, instr_b+4, 6);
  ALUfunc[6]="\0";
  //printf("ALU: %s", ALUfunc);
  ushort ALUtemp;
  ALU(&bussig->Dregout, &bussig->ALUy, ALUfunc, &ALUtemp);
  update_bsig(&bussig->ALUout, &ALUtemp);

}

void latch_pass(enum signalstate clk_phase){

  // only latch on rising edge
  if (clk_phase != clk_RE)
    return;

  printf("Latch pass\n");

  // Areg
  if (csig->loada == HI) {
    bussig->Aregout = bussig->ain;
    printf("A <- %x\n", bussig->ain);
  }
  if (csig->loadd == HI) {
    bussig->Dregout = bussig->ALUout;
    printf("D <- %x\n", bussig->ALUout);
  }

  // RAM
  if (csig->wren == HI) {
    writeram(bussig->Aregout, bussig->ALUout);
    printf("RAM[%x] <- %x\n", bussig->Aregout, bussig->ain);
  }

  // PC
  if (csig->jump == HI) {
    bussig->pc = bussig->Aregout;
  } else {
    bussig->pc += 1;
  }
  printf("PC <- %x\n", bussig->pc);
}


int update_csig(enum signalstate *csignal, enum signalstate state) {
  if (*csignal != state)
    updated = 1;
  *csignal = state;
}

int update_bsig(ushort *bsignal, ushort *value) {
  if (*bsignal != *value)
    updated = 1;
  *bsignal = *value;
}

void ALU(ushort *x, ushort *y, char *func, ushort *ALUout) {
  printf("func: %s\n", func);
  if (getbit6(func, 0)==1)
    *x = 0;
  if (getbit6(func, 1)==1)
    *x = ~(*x);
  if (getbit6(func, 2)==1)
    *y = 0;
  if (getbit6(func, 3)==1)
    *y = ~(*y);
  if (getbit6(func, 4)==1) {
    *ALUout = *x + *y;
  } else {
    *ALUout = *x & *y;
  }
  if (getbit6(func, 5)==1) {
    *ALUout = ~(*ALUout);
  }
  printf("Alu x: %d, ", *x);
  printf("Alu y: %d, ", *y);
  printf("Alu out: %d\n", *ALUout);
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

//C hello world example
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "units.h"
#include "types.h"



ushort ALU(ushort x, ushort y, char *func);
ushort readrom(ushort addr);
ushort readram(ushort addr);
void writeram(ushort addr, ushort value);
void signame_pass(ushort *clk);
void latch_pass(enum clkstate clk_phase);
int update_csig(enum control_sigs signame, enum signalstate state);
int update_bsig(int signame, ushort *value);
void setconsole(enum clkstate phase, int vflag);
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

int stdoutBackupFd;
FILE *nullOut;

int main(int argc,char *argv[])
{
  int cycle;
  int tempie;
  init();
  int instrcount;
  int maxticks = 0;

  /* duplicate stdout */
  stdoutBackupFd = dup(1);

  // Option parsing
  int vflag = 0;
  int bflag = 0;
  char *filename = NULL;
  int index;
  int c;

  opterr = 0;
  while ((c = getopt (argc, argv, "vf:t:")) != -1) {
    switch (c)
      {
      case 'v':
        vflag = 1;
        break;
      case 'f':
        filename = optarg;
        break;
      case 't':
        maxticks = atoi(optarg);
      case '?':
        if (optopt == 'f' || optopt == 't')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }
  }
//  printf ("vflag = %d, filename = %s\n",
//          vflag, filename);

  // Read the source
  FILE* file = fopen(filename, "r"); /* should check the result */
  char line[256];

  int addr = 0;
  while (fgets(line, sizeof(line), file)) {
    /* note that fgets don't strip the terminating \n, checking its
       presence would allow to handle lines longer that sizeof(line) */
    sscanf(line, "%hx", &rom[addr]);
    addr++;
  }

  if (maxticks == 0)
    maxticks = addr; // equal to program length

  // buffer[strcspn(buffer, "\n")] = 0;

  while(addr >= 0){
    printf("%x\n", rom[addr]);
    addr--;
  }


  // Main execution loop
  for(clk.tick = 0; clk.tick < maxticks; clk.tick++) {
    for(clk.phase=clk_LO; clk.phase<= clk_FE; clk.phase++){

      setconsole(clk.phase, vflag);

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
    setconsole(clk_RE, 0);
  }

  // Dump lower part of RAM
  printf("----------------------------RAM-----------------------------\n");
  int i;
  for(i=0;i<11;i++){
    printf("RAM[%04x]: %04x\n", i, readram(i));
  }
}

void init(void) {

  //csig = malloc(sizeof(struct control_sig));
  //alusig = malloc(sizeof(struct ALU_sig));
  //bussig = calloc(1, sizeof(struct bus_sig));
  bsig[PC] = 0;


  // initialize RAM
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

void latch_pass(enum clkstate clk_phase){

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

void setconsole(enum clkstate phase, int vflag) {
  if (vflag)
    return;

  if (phase != clk_RE ) {
    fflush(stdout);
    nullOut = fopen("/dev/null", "w");
    dup2(fileno(nullOut), 1);
  } else {
    fflush(stdout);
    fclose(nullOut);
    // Restore stdout
    dup2(stdoutBackupFd, 1);
    //close(stdoutBackupFd);
  }
}
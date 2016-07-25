//C hello world example
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "units.h"
#include "types.h"



ushort ALU(ushort x, ushort y, char *func);
ushort addradder(ushort x, ushort y);
ushort readrom(ushort addr);
ushort readram(ushort addr);
void writeram();
void signame_pass(ushort *clk);
void latch(enum clkstate clk_phase);
int update_csig(enum control_sigs signame, enum signalstate state);
int update_bsig(int signame, ushort *value);
int update_opsel(int opnr, int value);
int update_rfsel(int value);
void setconsole(enum clkstate phase, int vflag);
void init();
void reset_csig();
void writerf();

void generate_signals();


// Global variables
struct Areg *Areg;
struct Dreg *Dreg;
struct mux *ymux;
struct mux *imux;
ROM rom;
ROM *rom_p;
ushort ram[32768] = {0};
ushort *ramp_p;

// control signals
enum signalstate csig[16] = {0};
enum signalstate flags[16] = {0};


// buses
ushort bsig[20] = {0};
int op0_sel;
int op1_sel;
int op2_sel;
int op3_sel;
int dest_sel;
int rf_sel;

int icycle;


struct clk clk;

char ALUopc[3];
char ALUfunc[4];

int updated;
int stdoutBackupFd;
FILE *nullOut;

int main(int argc,char *argv[])
{
  int cycle;
  int tempie;
  init();
  int instrcount;
  int maxticks = 24;

  /* duplicate stdout */
  stdoutBackupFd = dup(1);

  // Option parsing
  int vflag = 0;
  int bflag = 0;
  char *filename = NULL;
  int index;
  int c;

  /*
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
        printf("Aborting..");
        abort ();
      }
  }
  */
  /*
  // Read the source
  FILE* file = fopen(filename, "r");
  char line[256];

  int addr = 0;
  while (fgets(line, sizeof(line), file)) {

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
*/

  // Main execution loop
  for(clk.tick = 0; clk.tick < maxticks; clk.tick++) {
    for(clk.phase=clk_FE; clk.phase<= clk_RE; clk.phase++){

      //setconsole(clk.phase, vflag);

      printf("------------------------------------------------------\n");
      printf("cycle: %d.%d, state: %d - PC: %x, MAR: %x, MDR: %x, RF_sel: %x\n", clk.tick, clk.phase, icycle, bsig[PC], bsig[MAR], bsig[MDR], rf_sel);
      printf("ALU: %x, zr: %x, ng: %x, \n", csig[MAR_SEL]);


      // signal generation phase
      updated = 1;
      cycle = 0;
      reset_csig();
      while(updated==1 && cycle < 15) {
        printf("Signal pass: ");
        updated = 0;
        cycle += 1;
        generate_signals();
        printf("\n");
      }
      printf("Stable after %d cycles.\n", cycle);
      printf("Alu x: %d, ", bsig[OP0]);
      printf("Alu y: %d, ", bsig[OP1]);
      printf("Alu out: %d\n", bsig[ALUout]);
      if(icycle == 1 || icycle == 2){
        printf("OP0: %s(%x)\n", BSIG_STRING[op0_sel], bsig[OP0]);
        printf("OP1: %s(%x)\n", BSIG_STRING[op1_sel], bsig[OP1]);
        printf("OP2: %s(%x)\n", BSIG_STRING[op2_sel], bsig[OP2]);
        printf("OP3: %s(%x)\n", BSIG_STRING[op3_sel], bsig[OP3]);
        //printf("OP2-enum: %d", OP1);
        printf("Dest: %s\n", BSIG_STRING[rf_sel]);
      }
      // Latch pass - single pass!
      latch(clk.phase);
    }
    //setconsole(clk_RE, 0);
  }
  /*
  // Dump lower part of RAM
  printf("----------------------------RAM-----------------------------\n");
  int i;
  for(i=0;i<11;i++){
    printf("RAM[%04x]: %04x\n", i, readram(i));
  }
  */
}

void init(void) {

  //csig = malloc(sizeof(struct control_sig));
  //alusig = malloc(sizeof(struct ALU_sig));
  //bussig = calloc(1, sizeof(struct bus_sig));
  bsig[PC] = 0;
  icycle = 0;

  // initialize RAM
  ram[0]=0x8011;
  ram[1]=0x801a;
  ram[2]=0xc653;
  ram[3]=0x4283;
  ram[4]=0x284;
  ram[5]=0xE451;

  printf("Init done..\n");
}


void
generate_signals() {

  ushort instr;
  ushort opc1;
  ushort opc2;
  ushort opc3;
  ushort opc4;
  ushort imm0;
  ushort imm1;
  ushort imm2;
  ushort imm3;
  ushort arg0;
  ushort arg1;
  ushort dest;
  ushort ir;
  ushort aluresult;
  ushort addradderresult;

  char *instr_b;
  char opc1_b[3];
  char opc2_b;
  char opc3_b[2];
  char opc4_b[3];
  char imm0_b[7];
  char imm1_b[10];
  char imm2_b[13];
  char imm3_b[3];
  char arg0_b[3];
  char arg1_b[3];
  char dest_b[3];
  char ir_b;

  int skipcond = 0;

  if(bsig[IR] == 0)
    return;

  printf("IR: %x, ", bsig[IR]);
  // parse instruction
  instr_b = decimal_to_binary16(bsig[IR]);
  printf("instruction: %s, ", instr_b);

  memcpy(ALUopc, instr_b+3, 3);

  memcpy(opc1_b, instr_b, 3);
  opc1 = bin3_to_dec(opc1_b);

  opc2_b = instr_b[3];
  opc2 = opc2_b - '0';

  memcpy(opc3_b, instr_b+4, 2);
  opc3 = bin2_to_dec(opc3_b);


  memcpy(opc4_b, instr_b+4, 3);
  opc4 = bin3_to_dec(opc4_b);

  // parse arguments - immediates
  memcpy(imm0_b, instr_b+3, 7);
  imm0 = bin7_to_dec(imm0_b);

  memcpy(imm1_b, instr_b+3, 10);
  imm1 = bin10_to_dec(imm1_b);

  memcpy(imm2_b, instr_b+3, 13);
  imm2 = bin13_to_dec(imm2_b);
  //printf("imm2: %s", imm2_b);

  memcpy(imm3_b, instr_b+7, 3);
  imm3 = bin3_to_dec(imm3_b);

  // parse arguments - operands
  memcpy(arg0_b, instr_b+7, 3);
  arg0 = bin3_to_dec(arg0_b);

  memcpy(arg1_b, instr_b+10, 3);
  arg1 = bin3_to_dec(arg1_b);

  memcpy(dest_b, instr_b+13, 3);
  dest = bin3_to_dec(dest_b);

  ir_b = instr_b[6];
  //printf("i/r: %c", ir_b);
  ir = ir_b - '0';


  // IRimm MUX
  switch(opc1) {
    case 0:
    case 1:
    case 2:
    case 3:
      update_bsig(IRimm, &imm0);
      break;
    case 4:
      update_bsig(IRimm, &imm1);
      break;
    case 5:
      update_bsig(IRimm, &imm2);
      break;
    case 6:
      update_bsig(IRimm, &imm3);
      break;
    case 7:
      if(opc3 == 1) {
        update_bsig(IRimm, &imm3);
      }
  }

  // MARin mux
  if(csig[MAR_SEL] == HI) {
    update_bsig(MARin, &bsig[ALUout]);
  } else {
    update_bsig(MARin, &bsig[PC]); // direct connection to PC
  }

  // MDR-ALU/RAM Mux
  if(csig[MDR_SEL] == HI) {
    ushort temp;
    temp =readram(bsig[MAR]);
    printf("MDR from RAM");
    update_bsig(MDRin, &temp);
  } else {
    update_bsig(MDRin, &bsig[ALUout]);
  }


  // set op1 and op2 MUX
  printf("opc2: %d", opc2);
  switch(opc1) {
  case 0:
    update_opsel(2, MDR); // 7bit immediate
    update_opsel(3, arg1);
    update_opsel(0, MDR);
    update_opsel(1, REG0);
    update_csig(MAR_SEL, HI);
    update_csig(MAR_LOAD, HI);
    update_csig(MDR_LOAD, HI);
    update_csig(MDR_SEL, HI);
    update_rfsel(dest);
    update_csig(RF_LOAD, HI);
    break;
  case 1:
  case 2:
    update_opsel(2, MDR); // 7bit immediate
    update_opsel(3, arg1);
    update_opsel(0, dest);
    update_opsel(1, REG0);
    update_csig(MAR_SEL, HI);
    update_csig(MAR_LOAD, HI);
    update_csig(MDR_LOAD, HI);
    update_csig(RAM_LOAD, HI);
    break;
  case 3:
    break;
  case 4:
    update_opsel(0, MDR);
    update_opsel(1, REG0);
    update_rfsel(dest);
    update_csig(RF_LOAD, HI);
    break;
  case 5:
    update_opsel(0, MDR);
    update_opsel(1, REG0);
    update_csig(PC_LOAD, HI);
    // op_sel: MDR, RF, MAR, FLAGS
    break;
  case 6:
    if(ir == 1) {
      update_opsel(0, MDR);
    } else {
      //printf("IR IS ZERO!!");
      update_opsel(0, arg0);
    }
    update_opsel(1, arg1);
    update_rfsel(dest);
    update_csig(MDR_SEL, HI);
    update_csig(RF_LOAD, HI);
    break;
  }
  if (opc1 == 7 && opc2 == 0) {
    switch(opc3) {
      case 1:
        // skip.c
        update_opsel(0, arg0);
        update_opsel(1, arg1);
        strcpy(ALUfunc, "101"); // subtract
        skipcond = 1;
        break;
    }
  }


  // determine ALUfunc
  if(opc1 < 6) {
    strcpy(ALUfunc, "001");
  }
  if(opc1 == 6) {
    strcpy(ALUfunc, ALUopc);
  }
  printf("ALUfunc: %s", ALUfunc);


  // ALU
  update_bsig(OP0, &bsig[op0_sel]);
  update_bsig(OP1, &bsig[op1_sel]);
  update_bsig(OP2, &bsig[op2_sel]);
  update_bsig(OP3, &bsig[op3_sel]);
  if (icycle == EXECUTE || icycle == MEM) {
    aluresult = ALU(bsig[OP0], bsig[OP1], &ALUfunc);
  } else {
    aluresult = ALU(bsig[OP2], bsig[OP3], &ALUfunc);
  }
  update_bsig(ALUout, &aluresult);

  // Condition testing
  // 000 EQ, 001 NEQ
  if (skipcond == 1) {
    switch(dest) {
      case 0:
        if(flags[ZR] == HI)
          update_csig(SKIP, HI);
        break;
      case 1:
        if(flags[ZR] != HI)
          update_csig(SKIP, HI);
        break;

    }
  }
}


void
latch(enum clkstate clk_phase) {

  //if(clk_phase == clk_LO || clk_phase == clk_HI)
    //return;

  // FETCH

  if(icycle == FETCH && clk_phase == clk_FE) {
    bsig[MAR] = bsig[PC];
    printf("MAR <- %x\n", bsig[MAR]);
  }

  if(icycle == FETCH && clk_phase == clk_RE) {
    bsig[IR] = readram(bsig[MAR]);
    printf("IR <- %x\n", readram(bsig[MAR]));
    bsig[PC]++; // PC acts as counter
  }

  // DECODE

  if(icycle == DECODE && clk_phase == clk_FE) {
    bsig[MDR] = bsig[IRimm];
    printf("MDR <- %x\n", bsig[MDR]);
  }
  if(icycle == DECODE && clk_phase == clk_RE) {
    if(csig[MAR_LOAD] == HI) {
      bsig[MAR] = bsig[MARin];
      printf("MAR <- %x\n", bsig[MAR]);
    }
  }


  // EXECUTE
  // prepare mdr and rf
  if(icycle == EXECUTE && clk_phase == clk_FE) {
    if(csig[MDR_LOAD] == HI) {
      bsig[MDR] = bsig[MDRin];
      printf("MDR <- %x\n", bsig[MDR]);
    }
    if(csig[PC_LOAD] == HI) {
      bsig[PC] = bsig[ALUout];
      printf("PC <- %x\n", bsig[ALUout]);
    }
    if(csig[SKIP] == HI) {
      bsig[PC]++;
      printf("PC <- skip\n");
    }
  }

  // MEM
  if(icycle == MEM && clk_phase == clk_FE) {
    if(csig[RAM_LOAD] == HI){
      writeram();
    }
    if(csig[RF_LOAD] == HI) {
      writerf();
    }
  }

  // progress state
  if(clk_phase == clk_RE) {
    if (icycle == MEM ) {
      icycle = FETCH;
      bsig[IR]=0;      // reset IR
    } else {
      icycle++;
    }
  }



}


ushort
ALU(ushort x, ushort y, char *func) {
  ushort result;

  if (getbit3(func, 0)==1)
    x = ~x;
  if (getbit3(func, 1)==1)
    y = ~y;
  if (getbit3(func, 2)==1) {
    result = x + y;
  } else {
    result = x & y;
  }
  if (getbit3(func, 0)==1) {
    result = ~result;
  }

  //set flags
  if (result == 0) {
    flags[ZR] = HI;
  } else {
    flags[ZR] = LO;
  }
  if (result < 0) {
    flags[NG] = HI;
  } else {
    flags[NG] = LO;
  }

  return result;
}

ushort
addradder(ushort x, ushort y) {
  return x + y;
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

int update_opsel(int opnr, int value) {
  //printf("signame: %d", signame);
  int *org;
  switch(opnr){
    case 0:
    org = &op0_sel;
    break;
    case 1:
    org = &op1_sel;
    break;
    case 2:
    org = &op2_sel;
    break;
    case 3:
    org = &op3_sel;
    break;
  }
  if (*org != value) {
    updated = 1;
    printf("op%d (%s), ", opnr, BSIG_STRING[value]);
    //if (signame == AIN)
      //printf("v: %x", *value);
  }
  *org = value;
}

int update_rfsel(int value) {
  if (rf_sel != value) {
    updated = 1;
    printf("rf_sel (%d), ", value);
  }
  rf_sel = value;
}


ushort readrom(ushort addr) {
  //printf("Reading ROM at address: %x\n", addr);
  return rom[addr];
}

ushort readram(ushort addr) {
  //printf("Reading RAM at address: %x\n", addr);
  return ram[addr];
}

void writeram() {
  ram[bsig[MAR]] = bsig[MDR];
  printf("RAM[%x] <- %x\n", bsig[MAR], bsig[MDR]);
}


void writerf() {
  bsig[rf_sel] = bsig[ALUout];
  printf("%s <- %x\n", BSIG_STRING[rf_sel], bsig[ALUout]);
}

void
reset_csig(){
  int i;
  for(i=0; i<16; i++ ){
    csig[i]= 0;
  }
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
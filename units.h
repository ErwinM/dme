#ifndef UNITS_H_   /* Include guard */
#define UNITS_H_

#include "types.h"


typedef ushort ROM[16834];
//typedef ushort RAM[32768];

enum signalstate { ZZ, RE, HI, FE, LO };
enum clkstate { clk_FE, clk_RE };

#define FETCH      0
#define DECODE     1
#define EXECUTE    2
#define MEM        3

enum control_sigs {MAR_LOAD, IR_LOAD, PC_LOAD, MDR_LOAD, MDR_SEL, RF_LOAD, MAR_SEL, RAM_LOAD, IMM_LOAD, SKIP};
static const char *CSIG_STRING[] = { "MAR_LOAD", "IR_LOAD", "PC_LOAD", "MDR_LOAD", "MDR_SEL", "RF_LOAD", "MAR_SEL", "RAM_LOAD", "IMM_LOAD", "SKIP" };

enum bus_sigs {  REG0, REGA, REGB, REGC, REGD, REGE, SP, PC, IR, MAR, MDR, OP0, OP1, OP2, OP3, CR, IRimm, MDRin, MARin, ALUout, NA=99};
static const char *BSIG_STRING[] = { "REG0", "REGA", "REGB", "REGC", "REGD", "REGE", "SP", "PC", "IR", "MAR", "MDR", "OP0", "OP1", "OP2", "OP3", "CR", "IRimm", "MDRin", "MARin", "ALUout" };

enum flags { ZR, NG};
static const char *FLAGS_STRING[] = { "ZR", "NG"};

//enum op_mux { selREG0, selREGA, selREGB, selREGC, selREGD, selREGE, selSP, selPC, selMAR, selMDR, selCR};
//static const char *OPMUX_STRING[] = {  "REG0", "REGA", "REGB", "REGC", "REGD", "REGE", "SP", "PC", "CR", "MAR", "MDR"};

struct clk {
  enum clkstate phase;
  int tick;
};

// Muxes / selects

#endif
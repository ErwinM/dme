#ifndef UNITS_H_   /* Include guard */
#define UNITS_H_

#include "types.h"


typedef ushort ROM[16834];
typedef ushort RAM[32768];

enum signalstate { ZZ, RE, HI, FE, LO };
enum clkstate { clk_LO, clk_RE, clk_HI, clk_FE};


enum control_sigs {SELAIN, LOADA, LOADD, SELY, JUMP, WREN, ZR, NG};
static const char *CSIG_STRING[] = { "selain", "loada", "loadd", "sely", "jump", "wren", "zr", "ng"};

enum bus_sigs {AIN, AREGOUT, ALUY, DREGOUT, ALUOUT, PC, INM};
static const char *BSIG_STRING[] = { "ain", "Aregout", "ALUy", "Dregout", "ALUout", "pc", "inM"};

/*
struct bus_sig {
  ushort ain;
  ushort Aregout;
  ushort ALUy;
  ushort Dregout;
  ushort ALUout;
  ushort pc;
  ushort inM;
};*/

struct clk {
  enum clkstate phase;
  int tick;
};

#endif
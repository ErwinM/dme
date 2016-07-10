#ifndef UNITS_H_   /* Include guard */
#define UNITS_H_

#include "types.h"


// ALU, PC and Decoder will be functions

struct Areg {
  ushort *in;
  ushort out;
  ushort *wren;
  ushort *clk;
};

struct Dreg {
  ushort *in;
  ushort out;
  ushort *wren;
  ushort *clk;
};

struct mux {
  ushort *sel;
  ushort *a;
  ushort *b;
  ushort out;
};

typedef ushort ROM[16834];
typedef ushort RAM[32768];

enum signalstate { ZZ, RE, HI, FE, LO };

struct control_sig {
  enum signalstate selain;
  enum signalstate loada;
  enum signalstate loadd;
  enum signalstate sely;
  enum signalstate ALUfunc;
  enum signalstate jump;
  enum signalstate wren;
  enum signalstate ALU1;
  enum signalstate ALU2;
  enum signalstate ALU3;
  enum signalstate ALU4;
  enum signalstate ALU5;
  enum signalstate ALU6;
};

struct ALU_sig {
  enum signalstate zr;
  enum signalstate ng;
};

struct bus_sig {
  ushort ain;
  ushort Aregout;
  ushort ALUy;
  ushort Dregout;
  ushort ALUout;
  ushort pc;
  ushort inM;
};

#endif
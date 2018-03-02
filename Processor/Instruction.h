// (C) 2018 University of Bristol. See License.txt

#ifndef _Instruction
#define _Instruction

/* Class to read and decode an instruction
 */

#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include "Processor/Data_Files.h"
#include "Networking/Player.h"
#include "Math/Integer.h"
#include "Auth/MAC_Check.h"

class Machine;
class Processor;

/* 
 * Opcode constants
 *
 * Whenever these are changed the corresponding dict in Compiler/instructions_base.py
 * MUST also be changed. (+ the documentation)
 */
enum
{
    // Load/store
    LDI = 0x1,
    LDSI = 0x2,
    LDMC = 0x3,
    LDMS = 0x4,
    STMC = 0x5,
    STMS = 0x6,
    LDMCI = 0x7,
    LDMSI = 0x8,
    STMCI = 0x9,
    STMSI = 0xA,
    MOVC = 0xB,
    MOVS = 0xC,
    PROTECTMEMS = 0xD,
    PROTECTMEMC = 0xE,
    PROTECTMEMINT = 0xF,
    LDMINT = 0xCA,
    STMINT = 0xCB,
    LDMINTI = 0xCC,
    STMINTI = 0xCD,
    PUSHINT = 0xCE,
    POPINT = 0xCF,
    MOVINT = 0xD0,
    // Machine
    LDTN = 0x10,
    LDARG = 0x11,
    REQBL = 0x12,
    STARG = 0x13,
    TIME = 0x14,
    START = 0x15,
    STOP = 0x16,
    USE = 0x17,
    USE_INP = 0x18,
    RUN_TAPE = 0x19,
    JOIN_TAPE = 0x1A,
    CRASH = 0x1B,
    USE_PREP = 0x1C,
    // Addition
    ADDC = 0x20,
    ADDS = 0x21,
    ADDM = 0x22,
    ADDCI = 0x23,
    ADDSI = 0x24,
    SUBC = 0x25,
    SUBS = 0x26,
    SUBML = 0x27,
    SUBMR = 0x28,
    SUBCI = 0x29,
    SUBSI = 0x2A,
    SUBCFI = 0x2B,
    SUBSFI = 0x2C,
    // Multiplication/division/other arithmetic
    MULC = 0x30,
    MULM = 0x31,
    MULCI = 0x32,
    MULSI = 0x33,
    DIVC = 0x34,
    DIVCI = 0x35,
    MODC = 0x36,
    MODCI = 0x37,
    LEGENDREC = 0x38,
    DIGESTC = 0x39,
    // Open
    STARTOPEN = 0xA0,
    STOPOPEN = 0xA1,
    // Data access
    TRIPLE = 0x50,
    BIT = 0x51,
    SQUARE = 0x52,
    INV = 0x53,
    INPUTMASK = 0x56,
    PREP = 0x57,
    // Input
    INPUT = 0x60,
    STARTINPUT = 0x61,
    STOPINPUT = 0x62,
    READSOCKETC = 0x63,
    READSOCKETS = 0x64,
    WRITESOCKETC = 0x65,
    WRITESOCKETS = 0x66,
    READSOCKETINT = 0x69,
    WRITESOCKETINT = 0x6a,
    WRITESOCKETSHARE = 0x6b,
    LISTEN = 0x6c,
    ACCEPTCLIENTCONNECTION = 0x6d,
    CONNECTIPV4 = 0x6e,
    READCLIENTPUBLICKEY = 0x6f,
    // Bitwise logic
    ANDC = 0x70,
    XORC = 0x71,
    ORC = 0x72,
    ANDCI = 0x73,
    XORCI = 0x74,
    ORCI = 0x75,
    NOTC = 0x76,
    // Bitwise shifts
    SHLC = 0x80,
    SHRC = 0x81,
    SHLCI = 0x82,
    SHRCI = 0x83,
    // Branching and comparison
    JMP = 0x90,
    JMPNZ = 0x91,
    JMPEQZ = 0x92,
    EQZC = 0x93,
    LTZC = 0x94,
    LTC = 0x95,
    GTC = 0x96,
    EQC = 0x97,
    JMPI = 0x98,
    // Integers
    LDINT = 0x9A,
    ADDINT = 0x9B,
    SUBINT = 0x9C,
    MULINT = 0x9D,
    DIVINT = 0x9E,
    PRINTINT = 0x9F,
    // Conversion
    CONVINT = 0xC0,
    CONVMODP = 0xC1,

    // IO
    PRINTMEM = 0xB0,
    PRINTREG = 0XB1,
    RAND = 0xB2,
    PRINTREGPLAIN = 0xB3,
    PRINTCHR = 0xB4,
    PRINTSTR = 0xB5,
    PUBINPUT = 0xB6,
    RAWOUTPUT = 0xB7,
    STARTPRIVATEOUTPUT = 0xB8,
    STOPPRIVATEOUTPUT = 0xB9,
    PRINTCHRINT = 0xBA,
    PRINTSTRINT = 0xBB,
    PRINTFLOATPLAIN = 0xBC,
    WRITEFILESHARE = 0xBD,     
    READFILESHARE = 0xBE,     

    // GF(2^n) versions
    
    // Load/store
    GLDI = 0x101,
    GLDSI = 0x102,
    GLDMC = 0x103,
    GLDMS = 0x104,
    GSTMC = 0x105,
    GSTMS = 0x106,
    GLDMCI = 0x107,
    GLDMSI = 0x108,
    GSTMCI = 0x109,
    GSTMSI = 0x10A,
    GMOVC = 0x10B,
    GMOVS = 0x10C,
    GPROTECTMEMS = 0x10D,
    GPROTECTMEMC = 0x10E,
    // Machine
    GREQBL = 0x112,
    GUSE_PREP = 0x11C,
    // Addition
    GADDC = 0x120,
    GADDS = 0x121,
    GADDM = 0x122,
    GADDCI = 0x123,
    GADDSI = 0x124,
    GSUBC = 0x125,
    GSUBS = 0x126,
    GSUBML = 0x127,
    GSUBMR = 0x128,
    GSUBCI = 0x129,
    GSUBSI = 0x12A,
    GSUBCFI = 0x12B,
    GSUBSFI = 0x12C,
    // Multiplication/division
    GMULC = 0x130,
    GMULM = 0x131,
    GMULCI = 0x132,
    GMULSI = 0x133,
    GDIVC = 0x134,
    GDIVCI = 0x135,
    GMULBITC = 0x136,
    GMULBITM = 0x137,
    // Open
    GSTARTOPEN = 0x1A0,
    GSTOPOPEN = 0x1A1,
    // Data access
    GTRIPLE = 0x150,
    GBIT = 0x151,
    GSQUARE = 0x152,
    GINV = 0x153,
    GBITTRIPLE = 0x154,
    GBITGF2NTRIPLE = 0x155,
    GINPUTMASK = 0x156,
    GPREP = 0x157,
    // Input
    GINPUT = 0x160,
    GSTARTINPUT = 0x161,
    GSTOPINPUT = 0x162,
    GREADSOCKETS = 0x164,
    GWRITESOCKETS = 0x166,
    // Bitwise logic
    GANDC = 0x170,
    GXORC = 0x171,
    GORC = 0x172,
    GANDCI = 0x173,
    GXORCI = 0x174,
    GORCI = 0x175,
    GNOTC = 0x176,
    // Bitwise shifts
    GSHLCI = 0x182,
    GSHRCI = 0x183,
    GBITDEC = 0x184,
    GBITCOM = 0x185,
    // Conversion
    GCONVINT = 0x1C0,
    GCONVGF2N = 0x1C1,
    // IO
    GPRINTMEM = 0x1B0,
    GPRINTREG = 0X1B1,
    GPRINTREGPLAIN = 0x1B3,
    GRAWOUTPUT = 0x1B7,
    GSTARTPRIVATEOUTPUT = 0x1B8,
    GSTOPPRIVATEOUTPUT = 0x1B9,
    // Commsec ops
    INITSECURESOCKET = 0x1BA,
    RESPSECURESOCKET = 0x1BB
};


// Register types
enum RegType {
  MODP,
  GF2N,
  INT,
  MAX_REG_TYPE,
  NONE
};

enum SecrecyType {
  SECRET,
  CLEAR,
  MAX_SECRECY_TYPE
};

struct TempVars {
  gf2n ans2; Share<gf2n> Sans2;
  gfp ansp;  Share<gfp>  Sansp;
  bigint aa,aa2;
  // INPUT and LDSI
  gfp rrp,tp,tmpp;
  gfp xip;
  // GINPUT and GLDSI
  gf2n rr2,t2,tmp2;
  gf2n xi2;
};


class BaseInstruction
{
protected:
  int opcode;         // The code
  int size;           // Vector size
  int r[4];           // Fixed parameter registers
  unsigned int n;     // Possible immediate value
  vector<int>  start; // Values for a start/stop open

public:
  virtual ~BaseInstruction() {};

  void parse_operands(istream& s, int pos);

  bool is_gf2n_instruction() const { return ((opcode&0x100)!=0); }
  virtual int get_reg_type() const;

  bool is_direct_memory_access(SecrecyType sec_type) const;

  // Returns the maximal register used
  int get_max_reg(int reg_type) const;
};


class Instruction : public BaseInstruction
{
public:
  // Reads a single instruction from the istream
  void parse(istream& s);

  // Return whether usage is known
  bool get_offline_data_usage(DataPositions& usage);

  // Returns the memory size used if applicable and known
  int get_mem(RegType reg_type, SecrecyType sec_type) const;

  friend ostream& operator<<(ostream& s,const Instruction& instr);

  // Execute this instruction, updateing the processor and memory
  // and streams pointing to the triples etc
  void execute(Processor& Proc) const;
};


#endif


// (C) 2016 University of Bristol. See License.txt

#ifndef _Program
#define _Program

#include "Processor/Instruction.h"
#include "Processor/Data_Files.h"

class Machine;

/* A program is a vector of instructions */

class Program
{
  vector<Instruction> p;
  // Here we note the number of bits, squares and triples and input
  // data needed
  //  - This is computed for a whole program sequence to enable
  //    the run time to be able to determine which ones to pass to it
  DataPositions offline_data_used;

  // Maximal register used
  int max_reg2,max_regp,max_regi;

  // Memory size used directly
  int max_mem[MAX_REG_TYPE][MAX_SECRECY_TYPE];

  // True if program contains variable-sized loop
  bool unknown_usage;

  void compute_constants();

  public:

  Program(int nplayers) : offline_data_used(nplayers),
      max_reg2(0), max_regp(0), max_regi(0), unknown_usage(false)
    { p.resize(0); }

  // Read in a program
  void parse(istream& s);

  DataPositions get_offline_data_used() const { return offline_data_used; }
  void print_offline_cost() const;

  bool usage_unknown() const { return unknown_usage; }

  int num_regs2() const         { return max_reg2; }
  int num_regsp() const         { return max_regp; }
  int num_regi() const          { return max_regi; }

  int direct_mem(RegType reg_type, SecrecyType sec_type)
    { return max_mem[reg_type][sec_type]; }

  int direct_mem2_s() const     { return max_mem[GF2N][SECRET]; }
  int direct_memp_s() const     { return max_mem[MODP][SECRET]; }
  int direct_mem2_c() const     { return max_mem[GF2N][CLEAR]; }
  int direct_memp_c() const     { return max_mem[MODP][CLEAR]; }
  int direct_memi_c() const     { return max_mem[INT][CLEAR]; }

  friend ostream& operator<<(ostream& s,const Program& P);

  // Execute this program, updateing the processor and memory
  // and streams pointing to the triples etc
  void execute(Processor& Proc) const;

};

#endif


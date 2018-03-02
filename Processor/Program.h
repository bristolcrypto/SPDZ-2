// (C) 2018 University of Bristol. See License.txt

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
  int max_reg[MAX_REG_TYPE];

  // Memory size used directly
  int max_mem[MAX_REG_TYPE][MAX_SECRECY_TYPE];

  // True if program contains variable-sized loop
  bool unknown_usage;

  void compute_constants();

  public:

  Program(int nplayers) : offline_data_used(nplayers),
      unknown_usage(false)
    { compute_constants(); }

  // Read in a program
  void parse(istream& s);

  DataPositions get_offline_data_used() const { return offline_data_used; }
  void print_offline_cost() const;

  bool usage_unknown() const { return unknown_usage; }

  int num_reg(RegType reg_type) const
    { return max_reg[reg_type]; }

  const int* direct_mem(RegType reg_type) const
    { return max_mem[reg_type]; }

  friend ostream& operator<<(ostream& s,const Program& P);

  // Execute this program, updateing the processor and memory
  // and streams pointing to the triples etc
  void execute(Processor& Proc) const;

};

#endif


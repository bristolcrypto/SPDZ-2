// (C) 2016 University of Bristol. See License.txt


#ifndef _Processor
#define _Processor

/* This is a representation of a processing element
 *   Consisting of 256 clear and 256 shared registers
 */

#include "Math/Share.h"
#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Integer.h"
#include "Exceptions/Exceptions.h"
#include "Networking/Player.h"
#include "Auth/MAC_Check.h"
#include "Data_Files.h"
#include "Input.h"
#include "PrivateOutput.h"
#include "Machine.h"

#include <stack>

class Processor
{
  vector<gf2n>  C2;
  vector<gfp>   Cp;
  vector<Share<gf2n> > S2;
  vector<Share<gfp> >  Sp;
  vector<long> Ci;
  
  // Stack
  stack<long> stacki;

  // This is the vector of partially opened values and shares we need to store
  // as the Open commands are split in two
  vector<gf2n> PO2;
  vector<gfp>  POp;
  vector<Share<gf2n> > Sh_PO2;
  vector<Share<gfp> >  Sh_POp;

  int reg_max2,reg_maxp,reg_maxi;
  int thread_num;

  // Optional argument to tape
  int arg;

  // For reading/reading data from a socket (i.e. external party to SPDZ)
  octetStream socket_stream;
  int socket_fd, final_socket_fd;
  bool socket_is_open;

  #ifdef DEBUG
    vector<int> rw2;
    vector<int> rwp;
    vector<int> rwi;
  #endif

  template <class T>
  vector< Share<T> >& get_S();
  template <class T>
  vector<T>& get_C();

  template <class T>
  vector< Share<T> >& get_Sh_PO();
  template <class T>
  vector<T>& get_PO();

  public:
  Data_Files& DataF;
  Player& P;
  MAC_Check<gf2n>& MC2;
  MAC_Check<gfp>& MCp;
  Machine& machine;

  Input<gf2n> input2;
  Input<gfp> inputp;
  
  PrivateOutput<gf2n> privateOutput2;
  PrivateOutput<gfp>  privateOutputp;

  ifstream public_input;
  ifstream private_input;
  ofstream public_output;
  ofstream private_output;

  unsigned int PC;
  TempVars temp;
  PRNG prng;

  int sent, rounds;

  static const int reg_bytes = 4;
  
  void reset(int num_regs2,int num_regsp,int num_regi,int arg); // Reset the state of the processor
  string get_filename(const char* basename, bool use_number);

  Processor(int thread_num,Data_Files& DataF,Player& P,
          MAC_Check<gf2n>& MC2,MAC_Check<gfp>& MCp,Machine& machine,
          int num_regs2 = 256,int num_regsp = 256,int num_regi = 256);
  ~Processor();

  int get_thread_num()
    {
      return thread_num;
    }

  int get_arg() const
    {
      return arg;
    }

  void set_arg(int new_arg)
    {
      arg=new_arg;
    }

  void pushi(long x) { stacki.push(x); }
  void popi(long& x) { x = stacki.top(); stacki.pop(); }

  #ifdef DEBUG  
    const gf2n& read_C2(int i) const
      { if (rw2[i]==0)
	  { throw Processor_Error("Invalid read on clear register"); }
        return C2.at(i);
      }
    const Share<gf2n> & read_S2(int i) const
      { if (rw2[i+reg_max2]==0)
          { throw Processor_Error("Invalid read on shared register"); }
        return S2.at(i);
      }
    gf2n& get_C2_ref(int i)
      { rw2[i]=1;
        return C2.at(i);
      }
    Share<gf2n> & get_S2_ref(int i)
      { rw2[i+reg_max2]=1;
        return S2.at(i);
      }
    void write_C2(int i,const gf2n& x)
      { rw2[i]=1;
        C2.at(i)=x;
      }
    void write_S2(int i,const Share<gf2n> & x)
      { rw2[i+reg_max2]=1;
        S2.at(i)=x;
      }

    const gfp& read_Cp(int i) const
      { if (rwp[i]==0)
	  { throw Processor_Error("Invalid read on clear register"); }
        return Cp.at(i);
      }
    const Share<gfp> & read_Sp(int i) const
      { if (rwp[i+reg_maxp]==0)
          { throw Processor_Error("Invalid read on shared register"); }
        return Sp.at(i);
      }
    gfp& get_Cp_ref(int i)
      { rwp[i]=1;
        return Cp.at(i);
      }
    Share<gfp> & get_Sp_ref(int i)
      { rwp[i+reg_maxp]=1;
        return Sp.at(i);
      }
    void write_Cp(int i,const gfp& x)
      { rwp[i]=1;
        Cp.at(i)=x;
      }
    void write_Sp(int i,const Share<gfp> & x)
      { rwp[i+reg_maxp]=1;
        Sp.at(i)=x;
      }

    const long& read_Ci(int i) const
      { if (rwi[i]==0)
          { throw Processor_Error("Invalid read on integer register"); }
        return Ci.at(i);
      }
    long& get_Ci_ref(int i)
      { rwi[i]=1;
        return Ci.at(i);
      }
    void write_Ci(int i,const long& x)
      { rwi[i]=1;
        Ci.at(i)=x;
      }
 #else
    const gf2n& read_C2(int i) const
      { return C2[i]; }
    const Share<gf2n> & read_S2(int i) const
      { return S2[i]; }
    gf2n& get_C2_ref(int i)
      { return C2[i]; }
    Share<gf2n> & get_S2_ref(int i)
      { return S2[i]; }
    void write_C2(int i,const gf2n& x)
      { C2[i]=x; }
    void write_S2(int i,const Share<gf2n> & x)
      { S2[i]=x; }
  
    const gfp& read_Cp(int i) const
      { return Cp[i]; }
    const Share<gfp> & read_Sp(int i) const
      { return Sp[i]; }
    gfp& get_Cp_ref(int i)
      { return Cp[i]; }
    Share<gfp> & get_Sp_ref(int i)
      { return Sp[i]; }
    void write_Cp(int i,const gfp& x)
      { Cp[i]=x; }
    void write_Sp(int i,const Share<gfp> & x)
      { Sp[i]=x; }

    const long& read_Ci(int i) const
      { return Ci[i]; }
    long& get_Ci_ref(int i)
      { return Ci[i]; }
    void write_Ci(int i,const long& x)
      { Ci[i]=x; }
  #endif

  // Template-based access
  template<class T> Share<T>& get_S_ref(int i);
  template<class T> T& get_C_ref(int i);

  // Access to sockets for reading clear/shared data
  void open_socket(int portnum_base);
  void close_socket();
  void read_socket(int& x);
  void write_socket(int x);
  template <class T>
  void read_socket(T& x);
  template <class T>
  void write_socket(const T& x);

  // Access to PO (via calls to POpen start/stop)
  template <class T>
  void POpen_Start(const vector<int>& reg,const Player& P,MAC_Check<T>& MC,int size);

  template <class T>
  void POpen_Stop(const vector<int>& reg,const Player& P,MAC_Check<T>& MC,int size);

  // Print the processor state
  friend ostream& operator<<(ostream& s,const Processor& P);
};

template<> inline Share<gf2n>& Processor::get_S_ref(int i) { return get_S2_ref(i); }
template<> inline gf2n& Processor::get_C_ref(int i)        { return get_C2_ref(i); }
template<> inline Share<gfp>& Processor::get_S_ref(int i)  { return get_Sp_ref(i); }
template<> inline gfp& Processor::get_C_ref(int i)         { return get_Cp_ref(i); }

template<> inline vector< Share<gf2n> >& Processor::get_S()       { return S2; }
template<> inline vector< Share<gfp> >& Processor::get_S()        { return Sp; }

template<> inline vector<gf2n>& Processor::get_C()                { return C2; }
template<> inline vector<gfp>& Processor::get_C()                 { return Cp; }

template<> inline vector< Share<gf2n> >& Processor::get_Sh_PO()   { return Sh_PO2; }
template<> inline vector<gf2n>& Processor::get_PO()               { return PO2; }
template<> inline vector< Share<gfp> >& Processor::get_Sh_PO()    { return Sh_POp; }
template<> inline vector<gfp>& Processor::get_PO()                { return POp; }

#endif


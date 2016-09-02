// (C) 2016 University of Bristol. See License.txt


#include "Processor/Processor.h"
#include "Auth/MAC_Check.h"

#include "Auth/fake-stuff.h"


Processor::Processor(int thread_num,Data_Files& DataF,Player& P,
        MAC_Check<gf2n>& MC2,MAC_Check<gfp>& MCp,Machine& machine,
        int num_regs2,int num_regsp,int num_regi)
: thread_num(thread_num),socket_is_open(false),DataF(DataF),P(P),MC2(MC2),MCp(MCp),machine(machine),
  input2(*this,MC2),inputp(*this,MCp),privateOutput2(*this),privateOutputp(*this),sent(0),rounds(0)
{
  reset(num_regs2,num_regsp,num_regi,0);

  public_input.open(get_filename("Programs/Public-Input/",false).c_str());
  private_input.open(get_filename("Player-Data/Private-Input-",true).c_str());
  public_output.open(get_filename("Player-Data/Public-Output-",true).c_str(), ios_base::out);
  private_output.open(get_filename("Player-Data/Private-Output-",true).c_str(), ios_base::out);
}


Processor::~Processor()
{
  cerr << "Sent " << sent << " elements in " << rounds << " rounds" << endl;
}


string Processor::get_filename(const char* prefix, bool use_number)
{
  stringstream filename;
  filename << prefix;
  if (!use_number)
    filename << machine.progname;
  if (use_number)
    filename << P.my_num();
  if (thread_num > 0)
    filename << "-" << thread_num;
  cerr << "Opening file " << filename.str() << endl;
  return filename.str();
}


void Processor::reset(int num_regs2,int num_regsp,int num_regi,int arg)
{
  reg_max2 = num_regs2;
  reg_maxp = num_regsp;
  reg_maxi = num_regi;
  C2.resize(reg_max2); Cp.resize(reg_maxp);
  S2.resize(reg_max2); Sp.resize(reg_maxp);
  Ci.resize(reg_maxi);
  this->arg = arg;
  close_socket();

  #ifdef DEBUG
    rw2.resize(2*reg_max2);
    for (int i=0; i<2*reg_max2; i++) { rw2[i]=0; }
    rwp.resize(2*reg_maxp);
    for (int i=0; i<2*reg_maxp; i++) { rwp[i]=0; }
    rwi.resize(2*reg_maxi);
    for (int i=0; i<2*reg_maxi; i++) { rwi[i]=0; }
  #endif
}

#include "Networking/sockets.h"

// Set up a server socket for some client
void Processor::open_socket(int portnum_base)
{
  if (!socket_is_open)
  {
    socket_is_open = true;
    sockaddr_in dest;
    set_up_server_socket(dest, final_socket_fd, socket_fd, portnum_base + P.my_num());
  }
}

void Processor::close_socket()
{
  if (socket_is_open)
  {
    socket_is_open = false;
    close_server_socket(final_socket_fd, socket_fd);
  }
}

// Receive 32-bit int
void Processor::read_socket(int& x)
{
  octet bytes[4];
  receive(final_socket_fd, bytes, 4);
  x = BYTES_TO_INT(bytes);
}

// Send 32-bit int
void Processor::write_socket(int x)
{
  octet bytes[4];
  INT_TO_BYTES(bytes, x);
  send(final_socket_fd, bytes, 4);
}

// Receive field element
template <class T>
void Processor::read_socket(T& x)
{
  socket_stream.reset_write_head();
  socket_stream.Receive(final_socket_fd);
  x.unpack(socket_stream);
}

// Send field element
template <class T>
void Processor::write_socket(const T& x)
{
  socket_stream.reset_write_head();
  x.pack(socket_stream);
  socket_stream.Send(final_socket_fd);
}

template <class T>
void Processor::POpen_Start(const vector<int>& reg,const Player& P,MAC_Check<T>& MC,int size)
{
  int sz=reg.size();
  vector< Share<T> >& Sh_PO = get_Sh_PO<T>();
  vector<T>& PO = get_PO<T>();
  Sh_PO.clear();
  Sh_PO.reserve(sz*size);
  if (size>1)
    {
      for (typename vector<int>::const_iterator reg_it=reg.begin();
          reg_it!=reg.end(); reg_it++)
        {
          typename vector<Share<T> >::iterator begin=get_S<T>().begin()+*reg_it;
          Sh_PO.insert(Sh_PO.end(),begin,begin+size);
        }
    }
  else
    {
      for (int i=0; i<sz; i++)
        { Sh_PO.push_back(get_S_ref<T>(reg[i])); }
    }
  PO.resize(sz*size);
  MC.POpen_Begin(PO,Sh_PO,P);
}


template <class T>
void Processor::POpen_Stop(const vector<int>& reg,const Player& P,MAC_Check<T>& MC,int size)
{
  vector< Share<T> >& Sh_PO = get_Sh_PO<T>();
  vector<T>& PO = get_PO<T>();
  vector<T>& C = get_C<T>();
  int sz=reg.size();
  PO.resize(sz*size);
  MC.POpen_End(PO,Sh_PO,P);
  if (size>1)
    {
      typename vector<T>::iterator PO_it=PO.begin();
      for (typename vector<int>::const_iterator reg_it=reg.begin();
          reg_it!=reg.end(); reg_it++)
        {
          for (typename vector<T>::iterator C_it=C.begin()+*reg_it;
              C_it!=C.begin()+*reg_it+size; C_it++)
            {
              *C_it=*PO_it;
              PO_it++;
            }
        }
    }
  else
    {
      for (unsigned int i=0; i<reg.size(); i++)
        { get_C_ref<T>(reg[i]) = PO[i]; }
    }

  sent += reg.size() * size;
  rounds++;
}







ostream& operator<<(ostream& s,const Processor& P)
{
  s << "Processor State" << endl;
  s << "Char 2 Registers" << endl;
  s << "Val\tClearReg\tSharedReg" << endl;
  for (int i=0; i<P.reg_max2; i++)
    { s << i << "\t";
      P.read_C2(i).output(s,true);
      s << "\t";
      P.read_S2(i).output(s,true);
      s << endl; 
    }
  s << "Char p Registers" << endl;
  s << "Val\tClearReg\tSharedReg" << endl;
  for (int i=0; i<P.reg_maxp; i++)
    { s << i << "\t";
      P.read_Cp(i).output(s,true);
      s << "\t";
      P.read_Sp(i).output(s,true);
      s << endl; 
    }

  return s;
}


template void Processor::POpen_Start(const vector<int>& reg,const Player& P,MAC_Check<gf2n>& MC,int size);
template void Processor::POpen_Start(const vector<int>& reg,const Player& P,MAC_Check<gfp>& MC,int size);
template void Processor::POpen_Stop(const vector<int>& reg,const Player& P,MAC_Check<gf2n>& MC,int size);
template void Processor::POpen_Stop(const vector<int>& reg,const Player& P,MAC_Check<gfp>& MC,int size);
template void Processor::read_socket(gfp& x);
template void Processor::read_socket(gf2n& x);
template void Processor::write_socket(const gfp& x);
template void Processor::write_socket(const gf2n& x);

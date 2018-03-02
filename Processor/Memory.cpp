// (C) 2018 University of Bristol. See License.txt

#include "Processor/Memory.h"
#include "Processor/Instruction.h"
#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Integer.h"

#include <fstream>

template<class T>
void Memory<T>::minimum_size(RegType reg_type, const Program& program, string threadname)
{
  const int* sizes = program.direct_mem(reg_type);
  if (sizes[SECRET] > size_s())
    {
      cerr << threadname << " needs more secret " << T::type_string() << " memory, resizing to "
          << sizes[SECRET] << endl;
      resize_s(sizes[SECRET]);
    }
  if (sizes[CLEAR] > size_c())
    {
      cerr << threadname << " needs more clear " << T::type_string() << " memory, resizing to "
          << sizes[CLEAR] << endl;
      resize_c(sizes[CLEAR]);
    }
}

#ifdef MEMPROTECT
template<class T>
void Memory<T>::protect_s(unsigned int start, unsigned int end)
{
  protected_s.insert(pair<unsigned int,unsigned int>(start, end));
}

template<class T>
void Memory<T>::protect_c(unsigned int start, unsigned int end)
{
  protected_c.insert(pair<unsigned int,unsigned int>(start, end));
}

template<class T>
bool Memory<T>::is_protected_s(unsigned int index)
{
  for (set< pair<unsigned int,unsigned int> >::iterator it = protected_s.begin();
      it != protected_s.end(); it++)
      if (it->first <= index and it->second > index)
        return true;
  return false;
}

template<class T>
bool Memory<T>::is_protected_c(unsigned int index)
{
  for (set< pair<unsigned int,unsigned int> >::iterator it = protected_c.begin();
      it != protected_c.end(); it++)
      if (it->first <= index and it->second > index)
        return true;
  return false;
}
#endif


template<class T>
ostream& operator<<(ostream& s,const Memory<T>& M)
{
  s << M.MS.size() << endl;
  s << M.MC.size() << endl;

#ifdef DEBUG
  for (unsigned int i=0; i<M.MS.size(); i++)
    { M.MS[i].output(s,true); s << endl; }
  s << endl;

  for (unsigned int i=0; i<M.MC.size(); i++)
    {  M.MC[i].output(s,true); s << endl; }
  s << endl;
#else
  for (unsigned int i=0; i<M.MS.size(); i++)
    { M.MS[i].output(s,false); }

  for (unsigned int i=0; i<M.MC.size(); i++)
    { M.MC[i].output(s,false); }
#endif

  return s;
}


template<class T>
istream& operator>>(istream& s,Memory<T>& M)
{
  int len;

  s >> len;  
  M.resize_s(len);
  s >> len;
  M.resize_c(len);
  s.seekg(1, istream::cur);

  for (unsigned int i=0; i<M.MS.size(); i++)
    { M.MS[i].input(s,false);  }

  for (unsigned int i=0; i<M.MC.size(); i++)
    { M.MC[i].input(s,false); }

  return s;
}


template<class T>
void Load_Memory(Memory<T>& M,ifstream& inpf)
{
  int a;
  T val;
  Share<T> S;

  inpf >> a;
  M.resize_s(a);
  inpf >> a;
  M.resize_c(a);

  cerr << "Reading Clear Memory" << endl;
  
  // Read clear memory
  inpf >> a;
  val.input(inpf,true);
  while (a!=-1)
    { M.write_C(a,val); 
      inpf >> a;  
      val.input(inpf,true); 
    }
  cerr << "Reading Shared Memory" << endl;

  // Read shared memory
  inpf >> a;
  S.input(inpf,true);
  while (a!=-1)
    { M.write_S(a,S); 
      inpf >> a;
      S.input(inpf,true);
    }
}

template class Memory<gfp>;
template class Memory<gf2n>;
template class Memory<Integer>;

template istream& operator>>(istream& s,Memory<gfp>& M);
template istream& operator>>(istream& s,Memory<gf2n>& M);
template istream& operator>>(istream& s,Memory<Integer>& M);

template ostream& operator<<(ostream& s,const Memory<gfp>& M);
template ostream& operator<<(ostream& s,const Memory<gf2n>& M);
template ostream& operator<<(ostream& s,const Memory<Integer>& M);

template void Load_Memory(Memory<gfp>& M,ifstream& inpf);
template void Load_Memory(Memory<gf2n>& M,ifstream& inpf);
template void Load_Memory(Memory<Integer>& M,ifstream& inpf);

#ifdef USE_GF2N_LONG
template class Memory<gf2n_short>;
template istream& operator>>(istream& s,Memory<gf2n_short>& M);
template ostream& operator<<(ostream& s,const Memory<gf2n_short>& M);
template void Load_Memory(Memory<gf2n_short>& M,ifstream& inpf);
#endif

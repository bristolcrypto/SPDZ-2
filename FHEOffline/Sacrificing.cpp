// (C) 2018 University of Bristol. See License.txt

/*
 * Checking.cpp
 *
 */

#include "Sacrificing.h"
#include "Producer.h"

#include "Auth/Subroutines.h"

// The number of sacrifices to amortize at one time
#define amortize 512


template<class T>
inline FileSacriFactory<T>::FileSacriFactory(const char* type, const Player& P,
        int output_thread)
{
    typename T::value_type dummy;
    /* Open file for reading in the initial triples */
    stringstream file1;
    file1 << PREP_DIR "Initial-" << type << "-" << file_completion(dummy)
            << "-P" << P.my_num();
    if (output_thread)
        file1 << "-" << output_thread;
    this->inpf.open(file1.str().c_str(),ios::in | ios::binary);
    if (this->inpf.fail()) { throw file_error(); }
}

template<class T>
inline void FileSacriFactory<T>::get(T& a, T& b, T& c)
{
    a.input(this->inpf, false);
    b.input(this->inpf, false);
    c.input(this->inpf, false);
}

template<class T>
inline void FileSacriFactory<T>::get(T& a, T& b)
{
    a.input(this->inpf, false);
    b.input(this->inpf, false);
}

template<class T>
inline void FileSacriFactory<T>::get(T& a)
{
    a.input(this->inpf, false);
}


void Triple_Inverse_Checking(const Player& P, MAC_Check<gfp>& MC, int nm,
    int nr, int output_thread)
{
  gfp dummy;
  printf("Doing Main Triple/Inverse Checking: %s\n",file_completion(dummy).c_str());

  FileSacriFactory< Share<gfp> > triple_factory("Triples", P, output_thread);
  FileSacriFactory< Share<gfp> > inverse_factory("Inverses", P, output_thread);
  Triple_Checking(P, MC, nm, output_thread, triple_factory);
  Inverse_Checking(P, MC, nr, output_thread, triple_factory, inverse_factory);

  printf("Finished Main Triple/Inverse Checking: %s\n",file_completion(dummy).c_str());
}

template <class T>
void Triple_Checking(const Player& P, MAC_Check<T>& MC, int nm,
    int output_thread, TripleSacriFactory< Share<T> >& factory, bool write_output,
    bool clear, string dir)
{
  ofstream outf;
  if (write_output)
    open_prep_file<T>(outf, "Triples", P.my_num(), output_thread, false,
        clear, dir);

  T te,t;
  Create_Random(t,P);
  vector<Share<T> > Sh_PO(2*amortize),Sh_Tau(amortize);
  vector<T> PO(2*amortize),Tau(amortize);
  vector<Share<T> > a1(amortize),b1(amortize),c1(amortize);
  vector<Share<T> > a2(amortize),b2(amortize),c2(amortize);
  Share<T> temp;

  // Triple checking
  int left_todo=nm; 
  while (left_todo>0)
    { int this_loop=amortize;
      if (this_loop>left_todo)
        { this_loop=left_todo;
          PO.resize(2*this_loop);
          Sh_PO.resize(2*this_loop);
          Tau.resize(this_loop);
          Sh_Tau.resize(this_loop);
        }
     
      for (int i=0; i<this_loop; i++)
        {
          factory.get(a1[i], b1[i], c1[i]);
          factory.get(a2[i], b2[i], c2[i]);
          Sh_PO[2*i].mul(a1[i],t);
          Sh_PO[2*i].sub(Sh_PO[2*i],a2[i]);
          Sh_PO[2*i+1].sub(b1[i],b2[i]);
	}
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { Sh_Tau[i].mul(c1[i],t);
          Sh_Tau[i].sub(Sh_Tau[i],c2[i]);
          temp.mul(a2[i],PO[2*i+1]);
          Sh_Tau[i].sub(Sh_Tau[i],temp);
          temp.mul(b2[i],PO[2*i]);
          Sh_Tau[i].sub(Sh_Tau[i],temp);
          te.mul(PO[2*i],PO[2*i+1]);
          Sh_Tau[i].sub(Sh_Tau[i],te,(P.my_num()==0),MC.get_alphai());
        }
      MC.POpen_Begin(Tau,Sh_Tau,P);
      MC.POpen_End(Tau,Sh_Tau,P);

      for (int i=0; i<this_loop; i++)
        { if (!Tau[i].is_zero())
	    { throw Offline_Check_Error("Multiplication Triples"); }
          if (write_output)
            {
              a1[i].output(outf,false);
              b1[i].output(outf,false);
              c1[i].output(outf,false);
            }
        }

      left_todo-=this_loop;
    }

  if (write_output)
    outf.close();
}


template <class T>
void Inverse_Checking(const Player& P, MAC_Check<T>& MC, int nr,
    int output_thread, TripleSacriFactory<Share<T> >& triple_factory,
    TupleSacriFactory<Share<T> >& inverse_factor, bool write_output,
    bool clear, string dir)
{
  ofstream outf_inv;
  if (write_output)
    open_prep_file<T>(outf_inv, "Inverses", P.my_num(), output_thread, false,
        clear, dir);

  T te,t;
  Create_Random(t,P);
  vector<Share<T> > Sh_PO(2*amortize),Sh_Tau(amortize);
  vector<T> PO(2*amortize),Tau(amortize);
  vector<Share<T> > a1(amortize),b1(amortize),c1(amortize);
  vector<Share<T> > a2(amortize),b2(amortize),c2(amortize);
  Share<T> temp;

  // Inverse checking
  int left_todo=nr;
  while (left_todo>0)
    { int this_loop=amortize;
      if (this_loop>left_todo)
        { this_loop=left_todo;
          PO.resize(2*this_loop);
          Sh_PO.resize(2*this_loop);
          Tau.resize(this_loop);
          Sh_Tau.resize(this_loop);
        }

      for (int i=0; i<this_loop; i++)
        {
          inverse_factor.get(a1[i], b1[i]);
          triple_factory.get(a2[i], b2[i], c2[i]);
          Sh_PO[2*i].mul(a1[i],t);
          Sh_PO[2*i].sub(Sh_PO[2*i],a2[i]);
          Sh_PO[2*i+1].sub(b1[i],b2[i]);
        }
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        {
          Sh_Tau[i].sub(c2[i],t,(P.my_num()==0),MC.get_alphai());
          temp.mul(a2[i],PO[2*i+1]);
          Sh_Tau[i].add(Sh_Tau[i],temp);
          temp.mul(b1[i],PO[2*i]);
          Sh_Tau[i].add(Sh_Tau[i],temp);
        }
      MC.POpen_Begin(Tau,Sh_Tau,P);
      MC.POpen_End(Tau,Sh_Tau,P);

      for (int i=0; i<this_loop; i++)
        { if (!Tau[i].is_zero())
            { throw Offline_Check_Error("Inverses"); }
          if (write_output)
            {
              a1[i].output(outf_inv,false);
              b1[i].output(outf_inv,false);
            }
        }

      left_todo-=this_loop;
    }

  if (write_output)
    {
      outf_inv.close();
    }
}


void Triple_Checking(const Player& P,MAC_Check<gf2n_short>& MC,int nm)
{
  gf2n_short dummy;
  printf("Doing Main Triple Checking: %s\n",file_completion(dummy).c_str());

  /* Open file for reading in the initial triples */
  stringstream file1; file1 << PREP_DIR "Initial-Triples-" << file_completion(dummy) << "-P" << P.my_num();
  ifstream inpf(file1.str().c_str(),ios::in | ios::binary);
  if (inpf.fail()) { throw file_error(); }

  /* Open file for writing out the final triples */
  stringstream file3; file3 << PREP_DIR "Triples-" << file_completion(dummy) << "-P" << P.my_num();
  ofstream outf(file3.str().c_str(),ios::out | ios::binary);
  if (outf.fail()) { throw file_error(); }

  gf2n_short te,t;
  Create_Random(t,P);
  vector<Share<gf2n_short> > Sh_PO(2*amortize),Sh_Tau(amortize);
  vector<gf2n_short> PO(2*amortize),Tau(amortize);
  vector<Share<gf2n_short> > a1(amortize),b1(amortize),c1(amortize);
  vector<Share<gf2n_short> > a2(amortize),b2(amortize),c2(amortize);
  Share<gf2n_short> temp;

  // Triple checking
  int left_todo=nm; 
  while (left_todo>0)
    { int this_loop=amortize;
      if (this_loop>left_todo)
        { this_loop=left_todo;
          PO.resize(2*this_loop);
          Sh_PO.resize(2*this_loop);
          Tau.resize(this_loop);
          Sh_Tau.resize(this_loop);
        }
     
      for (int i=0; i<this_loop; i++)
        { a1[i].input(inpf,false);
          b1[i].input(inpf,false);
          c1[i].input(inpf,false);
          a2[i].input(inpf,false);
          b2[i].input(inpf,false);
          c2[i].input(inpf,false);
          Sh_PO[2*i].mul(a1[i],t);
          Sh_PO[2*i].sub(Sh_PO[2*i],a2[i]);
          Sh_PO[2*i+1].sub(b1[i],b2[i]);
	}
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { Sh_Tau[i].mul(c1[i],t);
          Sh_Tau[i].sub(Sh_Tau[i],c2[i]);
          temp.mul(a2[i],PO[2*i+1]);
          Sh_Tau[i].sub(Sh_Tau[i],temp);
          temp.mul(b2[i],PO[2*i]);
          Sh_Tau[i].sub(Sh_Tau[i],temp);
          te.mul(PO[2*i],PO[2*i+1]);
          Sh_Tau[i].sub(Sh_Tau[i],te,(P.my_num()==0),MC.get_alphai());
        }
      MC.POpen_Begin(Tau,Sh_Tau,P);
      MC.POpen_End(Tau,Sh_Tau,P);

      for (int i=0; i<this_loop; i++)
        { if (!Tau[i].is_zero())
	    { throw Offline_Check_Error("Multiplication Triples"); }
          a1[i].output(outf,false);
          b1[i].output(outf,false);
          c1[i].output(outf,false);
        }

      left_todo-=this_loop;
    }

  outf.close();
  inpf.close();
  printf("Finished Main Triple Checking: %s\n",file_completion(dummy).c_str());
}


void Square_Bit_Checking(const Player& P,MAC_Check<gfp>& MC,int ns,int nb)
{
  gfp dummy;
  printf("Doing Main Square/Bit Checking: %s\n",file_completion(dummy).c_str());

  FileSacriFactory< Share<gfp> > square_factory("Squares", P, 0);
  FileSacriFactory< Share<gfp> > bit_factory("Bits", P, 0);
  Square_Checking(P, MC, ns, 0, square_factory, true);
  Bit_Checking(P, MC, nb, 0, square_factory, bit_factory, true);

  printf("Finished Main Square/Bit Checking: %s\n",file_completion(dummy).c_str());
}

template <class T>
void Square_Checking(const Player& P, MAC_Check<T>& MC, int ns,
        int output_thread, TupleSacriFactory<Share<T> >& square_factory,
        bool write_output, bool clear, string dir)
{
  ofstream outf_s, outf_b;
  if (write_output)
  {
    open_prep_file<T>(outf_s, "Squares", P.my_num(), output_thread, false, clear, dir);
  }

  T te,t,t2;
  Create_Random(t,P);
  t2.mul(t,t);
  vector<Share<T> > Sh_PO(amortize);
  vector<T> PO(amortize);
  vector<Share<T> > f(amortize),h(amortize),a(amortize),b(amortize);
  Share<T>  temp;

  // Do the square checking
  int left_todo=ns;
   while (left_todo>0)
    { int this_loop=amortize;
      if (this_loop>left_todo)
        { this_loop=left_todo;
          PO.resize(this_loop);
          Sh_PO.resize(this_loop);
        }

      for (int i=0; i<this_loop; i++)
        {
          square_factory.get(f[i], h[i]);
          square_factory.get(a[i], b[i]);

          Sh_PO[i].mul(a[i],t);
          Sh_PO[i].sub(Sh_PO[i],f[i]);
        }
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { Sh_PO[i].mul(b[i],t2);
          Sh_PO[i].sub(Sh_PO[i],h[i]);
          temp.mul(a[i],t);
          temp.add(temp,f[i]);
          temp.mul(temp,PO[i]);
          Sh_PO[i].sub(Sh_PO[i],temp);
        }
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { if (!PO[i].is_zero())
            { throw Offline_Check_Error("Squares"); }
          if (write_output)
            {
              a[i].output(outf_s,false); b[i].output(outf_s,false);
            }
        }
      left_todo-=this_loop;
    }
  outf_s.close(); 
}

void Bit_Checking(const Player& P, MAC_Check<gfp>& MC, int nb,
        int output_thread, TupleSacriFactory<Share<gfp> >& square_factory,
        SingleSacriFactory<Share<gfp> >& bit_factory, bool write_output,
        bool clear, string dir)
{
  gfp dummy;
  ofstream outf_b;
  if (write_output)
    open_prep_file<gfp>(outf_b, "Bits", P.my_num(), output_thread, false, clear,
        dir);

  gfp te,t,t2;
  Create_Random(t,P);
  t2.mul(t,t);
  vector<Share<gfp> > Sh_PO(amortize);
  vector<gfp> PO(amortize);
  vector<Share<gfp> > f(amortize),h(amortize),a(amortize),b(amortize);
  Share<gfp>  temp;

  // Do the bits checking
  PO.resize(amortize);
  Sh_PO.resize(amortize);
  int left_todo=nb;
  while (left_todo>0)
    { int this_loop=amortize;
      if (this_loop>left_todo)
	{ this_loop=left_todo;
          PO.resize(this_loop);
          Sh_PO.resize(this_loop);
	}

      for (int i=0; i<this_loop; i++)
        {
          square_factory.get(f[i], h[i]);
          bit_factory.get(a[i]);

          Sh_PO[i].mul(a[i],t);
          Sh_PO[i].sub(Sh_PO[i],f[i]);
	}
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { Sh_PO[i].mul(a[i],t2);
          Sh_PO[i].sub(Sh_PO[i],h[i]);
          temp.mul(a[i],t);
          temp.add(temp,f[i]);
          temp.mul(temp,PO[i]);
          Sh_PO[i].sub(Sh_PO[i],temp);
        }
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { if (!PO[i].is_zero())
            { throw Offline_Check_Error("Bits"); }
          if (write_output)
            a[i].output(outf_b,false);
	}

      left_todo-=this_loop;
    }
  outf_b.close();
}



void Square_Checking(const Player& P,MAC_Check<gf2n_short>& MC,int ns)
{
  gf2n_short dummy;
  printf("Doing Main Square Checking: %s\n",file_completion(dummy).c_str());

  /* Open files for reading in the initial data */
  stringstream file1; file1 << PREP_DIR "Initial-Squares-" << file_completion(dummy) << "-P" << P.my_num();
  ifstream inpf_s(file1.str().c_str(),ios::in | ios::binary);
  if (inpf_s.fail()) { throw file_error(); }

  /* Open files for writing out the final data */
  stringstream file3; file3 << PREP_DIR "Squares-" << file_completion(dummy) << "-P" << P.my_num();
  ofstream outf_s(file3.str().c_str(),ios::out | ios::binary);
  if (outf_s.fail()) { throw file_error(); }

  gf2n_short te,t,t2;
  Create_Random(t,P);
  t2.mul(t,t);
  vector<Share<gf2n_short> > Sh_PO(amortize);
  vector<gf2n_short> PO(amortize);
  vector<Share<gf2n_short> > f(amortize),h(amortize),a(amortize),b(amortize);
  Share<gf2n_short>  temp;

  // Do the square checking
  int left_todo=ns;
   while (left_todo>0)
    { int this_loop=amortize;
      if (this_loop>left_todo)
        { this_loop=left_todo;
          PO.resize(this_loop);
          Sh_PO.resize(this_loop);
        }

      for (int i=0; i<this_loop; i++)
        { f[i].input(inpf_s,false);
          h[i].input(inpf_s,false);
          a[i].input(inpf_s,false);
          b[i].input(inpf_s,false);

          Sh_PO[i].mul(a[i],t);
          Sh_PO[i].sub(Sh_PO[i],f[i]);
        }
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { Sh_PO[i].mul(b[i],t2);
          Sh_PO[i].sub(Sh_PO[i],h[i]);
          temp.mul(a[i],t);
          temp.add(temp,f[i]);
          temp.mul(temp,PO[i]);
          Sh_PO[i].sub(Sh_PO[i],temp);
        }
      MC.POpen_Begin(PO,Sh_PO,P);
      MC.POpen_End(PO,Sh_PO,P);

      for (int i=0; i<this_loop; i++)
        { if (!PO[i].is_zero())
            { throw Offline_Check_Error("Squares"); }
          a[i].output(outf_s,false); b[i].output(outf_s,false); 
        }
      left_todo-=this_loop;
    }
  outf_s.close(); 
  inpf_s.close(); 
  printf("Finished Main Square Checking: %s\n",file_completion(dummy).c_str());
}

template void Triple_Checking(const Player& P, MAC_Check<gfp>& MC, int nm,
        int output_thread, TripleSacriFactory<Share<gfp> >& factory,
        bool write_output, bool clear, string dir);
template void Triple_Checking(const Player& P, MAC_Check<gf2n_short>& MC,
        int nm, int output_thread,
        TripleSacriFactory<Share<gf2n_short> >& factory, bool write_output,
        bool clear, string dir);

template void Square_Checking(const Player& P, MAC_Check<gfp>& MC, int ns,
        int output_thread, TupleSacriFactory<Share<gfp> >& square_factory,
        bool write_output, bool clear, string dir);
template void Square_Checking(const Player& P, MAC_Check<gf2n_short>& MC,
        int ns, int output_thread,
        TupleSacriFactory<Share<gf2n_short> >& square_factory,
        bool write_output, bool clear, string dir);

template void Inverse_Checking(const Player& P, MAC_Check<gfp>& MC, int nr,
        int output_thread, TripleSacriFactory<Share<gfp> >& triple_factory,
        TupleSacriFactory<Share<gfp> >& inverse_factor, bool write_output,
        bool clear, string dir);
template void Inverse_Checking(const Player& P, MAC_Check<gf2n_short>& MC, int nr,
        int output_thread, TripleSacriFactory<Share<gf2n_short> >& triple_factory,
        TupleSacriFactory<Share<gf2n_short> >& inverse_factor, bool write_output,
        bool clear, string dir);

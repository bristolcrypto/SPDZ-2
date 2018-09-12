// (C) 2018 University of Bristol. See License.txt


#include "FHE/P2Data.h"
#include "Math/Setup.h"
#include <fstream>


void P2Data::forward(vector<int>& ans,const vector<gf2n_short>& a) const
{
  int n=gf2n_short::degree();
  
  ans.resize(A.size());
  for (unsigned i=0; i<A.size(); i++)
    { ans[i]=0; }
  for (int i=0; i<slots; i++)
    { word rep=a[i].get();
      for (int j=0; j<n && rep!=0; j++)
        { if ((rep&1)==1)
	     { int jj=i*n+j;
               for (unsigned k=0; k<A.size(); k++)
	          { if (A[k][jj]==1) { ans[k]=ans[k]^1; } }
             }
          rep>>=1;
        }
    }
}


void P2Data::backward(vector<gf2n_short>& ans,const vector<int>& a) const
{
  int n=gf2n_short::degree();
  
  ans.resize(slots);
  word y;
  for (int i=0; i<slots; i++)
    { y=0;
      for (int j=0; j<n; j++)
        { y<<=1;
          int ii=i*n+n-1-j,xx=0;
          for (unsigned int k=0; k<a.size(); k++)
            { if ((a[k] & 1) && Ai[ii][k]==1) { xx^=1; } }
          y^=xx;
        }
      ans[i].assign(y);
    }
}


void P2Data::check_dimensions() const
{
//  cout << "degree: " << gf2n::degree() << endl;
//  cout << "slots: " << slots << endl;
//  cout << "A: " << A.size() << "x" << A[0].size() << endl;
//  cout << "Ai: " << Ai.size() << "x" << Ai[0].size() << endl;
  if (A.size() != Ai.size())
    throw runtime_error("forward and backward mapping dimensions mismatch");
  if (A.size() != A[0].size())
    throw runtime_error("forward mapping not square");
  if (Ai.size() != Ai[0].size())
    throw runtime_error("backward mapping not square");
  if ((int)A[0].size() != slots * gf2n::degree())
    throw runtime_error("mapping dimension incorrect");
}


bool P2Data::operator!=(const P2Data& other) const
{
  return slots != other.slots or A != other.A or Ai != other.Ai;
}

void P2Data::pack(octetStream& o) const
{
  check_dimensions();
  o.store(slots);
  A.pack(o);
  Ai.pack(o);
}

void P2Data::unpack(octetStream& o)
{
  o.get(slots);
  A.unpack(o);
  Ai.unpack(o);
  check_dimensions();
}

ostream& operator<<(ostream& s,const P2Data& P2D)
{
  P2D.check_dimensions();
  s << P2D.slots << endl;
  s << P2D.A << endl;
  s << P2D.Ai << endl;
  return s;
}

istream& operator>>(istream& s,P2Data& P2D)
{
  s >> P2D.slots;
  s >> P2D.A;
  s >> P2D.Ai;
  P2D.check_dimensions();
  return s;
}

string get_filename(const Ring& Rg)
{
  return (string) PREP_DIR + "P2D-" + to_string(gf2n::degree()) + "x"
      + to_string(Rg.phi_m() / gf2n::degree());
}

void P2Data::load(const Ring& Rg)
{
  string filename = get_filename(Rg);
  cout << "Loading from " << filename << endl;
  ifstream s(filename);
  octetStream os;
  os.input(s);
  if (s.eof() or s.fail())
    throw runtime_error("cannot load P2Data");
  unpack(os);
}

void P2Data::store(const Ring& Rg) const
{
  string filename = get_filename(Rg);
  cout << "Storing in " << filename << endl;
  ofstream s(filename);
  octetStream os;
  pack(os);
  os.output(s);
}

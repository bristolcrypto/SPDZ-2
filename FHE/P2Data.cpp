// (C) 2017 University of Bristol. See License.txt


#include "FHE/P2Data.h"


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
            { if (a[k]==1 && Ai[ii][k]==1) { xx^=1; } }
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

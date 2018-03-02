// (C) 2018 University of Bristol. See License.txt

#include "Math/Subroutines.h"
#include "FHE/PPData.h"
#include "FHE/FFT.h"
#include "FHE/Matrix.h"



void PPData::assign(const PPData& PPD)
{
  R=PPD.R;
  prData=PPD.prData;
  root=PPD.root;
}


void PPData::init(const Ring& Rg,const Zp_Data& PrD)
{
  R=Rg;
  prData=PrD;

  root=Find_Primitive_Root_m(Rg.m(),Rg.Phi(),PrD);
}
    

ostream& operator<<(ostream& s,const PPData& PPD)
{
  bigint ans;
  s << PPD.prData << endl;
  s << PPD.R << endl;
  to_bigint(ans,PPD.root,PPD.prData); s << ans << " ";

  return s;
}


istream& operator>>(istream& s,PPData& PPD)
{
  bigint ans;
  s >> PPD.prData >> PPD.R;
  s >> ans; to_modp(PPD.root,ans,PPD.prData);
  return s;
}



void PPData::to_eval(vector<modp>& elem) const
{
  if (elem.size()!= (unsigned) R.phi_m())
    { throw params_mismatch(); }

  throw not_implemented();

/*
  vector<modp> ans; 
  ans.resize(R.phi_m());
  modp x=root;
  for (int i=0; i<R.phi_m(); i++)
    { ans[i]=elem[R.phi_m()-1];
      for (int j=1; j<R.phi_m(); j++)
	{ Mul(ans[i],ans[i],x,prData);
          Add(ans[i],ans[i],elem[R.phi_m()-j-1],prData);
	}
      Mul(x,x,root,prData);
    }
  elem=ans;
*/
}

void PPData::from_eval(vector<modp>& elem) const
{
  // avoid warning
  elem.empty();
  throw not_implemented();

  /*
  modp_matrix A;
  int n=phi_m();
  A.resize(n, vector<modp>(n+1) );
  modp x=root;
  for (int i=0; i<n; i++)
    { assignOne(A[0][i],prData);
      for (int j=1; j<n; j++)
         { Mul(A[j][i],A[j-1][i],x,prData); }
      Mul(x,x,root,prData);
      A[i][n]=elem[i];
    }
  elem=solve(A,prData);
  */
	
}


void PPData::reset_iteration()
{
  pow=1; theta.assign(root); thetaPow=theta; 
}

void PPData::next_iteration()
{
  do
    { thetaPow.mul(theta);
      pow++;
    }
  while (gcd(pow,m())!=1);


}


gfp PPData::get_evaluation(const vector<bigint>& mess) const
{
  // Uses Horner's rule
  gfp ans;
  to_gfp(ans,mess[mess.size()-1]);
  gfp coeff;
  for (int j=mess.size()-2; j>=0; j--)
        { ans.mul(thetaPow);
          to_gfp(coeff,mess[j]);
          ans.add(coeff);
        }
  return ans;
}



// (C) 2018 University of Bristol. See License.txt


#include "DiscreteGauss.h"
#include "math.h"

void DiscreteGauss::set(double RR)
{
  R=RR;
  e=exp(1); e1=exp(0.25); e2=exp(-1.35); 
}



/*  Return a value distributed normaly with std dev R */
int DiscreteGauss::sample(PRNG& G, int stretch) const
{
  /*  Uses the ratio method from Wikipedia to get a 
       Normal(0,1) variable X
       Then multiplies X by R
  */
  double U,V,X,R1,R2,R3,X2;
  int ans;
  while (true)
    { U=G.get_double();
      V=G.get_double();
      R1=5-4*e1*U;
      R2=4*e2/U+1.4;
      R3=-4/log(U);
      X=sqrt(8/e)*(V-0.5)/U;
      X2=X*X;
      if (X2<=R1 || (X2<R2 && X2<=R3))
        { ans=(int) (X*R*stretch);
          return ans;
        }
    }
}



void RandomVectors::set(int nn,int hh,double R)
{
  n=nn;
  h=hh;
  DG.set(R);
}


 
vector<bigint> RandomVectors::sample_Gauss(PRNG& G, int stretch) const
{
  vector<bigint> ans(n);
  for (int i=0; i<n; i++)
    { ans[i]=DG.sample(G, stretch); }
  return ans;
}


vector<bigint> RandomVectors::sample_Hwt(PRNG& G) const
{
  if (h>n/2) { return sample_Gauss(G); }
  vector<bigint> ans(n);
  for (int i=0; i<n; i++) { ans[i]=0; }
  int cnt=0,j=0;
  unsigned char ch=0;
  while (cnt<h)
    { unsigned int i=G.get_uint()%n;
      if (ans[i]==0)
	{ cnt++;
          if (j==0)
            { j=8; 
              ch=G.get_uchar();
            }
          int v=ch&1; j--;
          if (v==0) { ans[i]=-1; }
          else      { ans[i]=1;  }
        }
    }
  return ans;
}




int sample_half(PRNG& G)
{
  int v=G.get_uchar()&3;
  if (v==0 || v==1)
    return 0;
  else if (v==2)
    return 1;
  else
    return -1;
}


vector<bigint> RandomVectors::sample_Half(PRNG& G) const
{
  vector<bigint> ans(n);
  for (int i=0; i<n; i++)
    ans[i] = sample_half(G);
  return ans;
}


vector<bigint> RandomVectors::sample_Uniform(PRNG& G,const bigint& B) const
{
  vector<bigint> ans(n);
  bigint v;
  for (int i=0; i<n; i++)
    { G.get_bigint(v, numBits(B));
      int bit=G.get_uint()&1;
      if (bit==0) { ans[i]=v; }
      else        { ans[i]=-v; }
    }
  return ans;
}

bool RandomVectors::operator!=(const RandomVectors& other) const
{
  if (n != other.n or h != other.h or DG != other.DG)
    return true;
  else
    return false;
}

bool DiscreteGauss::operator!=(const DiscreteGauss& other) const
{
  if (other.R != R or other.e != e or other.e1 != e1 or other.e2 != e2)
    return true;
  else
    return false;
}

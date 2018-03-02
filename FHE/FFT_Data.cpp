// (C) 2018 University of Bristol. See License.txt

#include "FHE/FFT_Data.h"
#include "FHE/FFT.h"

#include "Math/Subroutines.h"


void FFT_Data::assign(const FFT_Data& FFTD)
{
  prData=FFTD.prData;
  R=FFTD.R;

  root=FFTD.root;
  twop=FFTD.twop;

  two_root=FFTD.two_root;
  powers=FFTD.powers;
  powers_i=FFTD.powers_i;
  b=FFTD.b;

  iphi=FFTD.iphi;

}



void FFT_Data::init(const Ring& Rg,const Zp_Data& PrD)
{
  R=Rg;
  prData=PrD;
  root.resize(2);

  // Find which case we are in 
  int hwt=Hwt(Rg.m());
  bigint prm1=PrD.pr-1;
  twop=1<<(numBits(Rg.m())+1);
  if (hwt==1)
    { twop=0; }
  else if ((prm1%twop)!=0)
    { twop=-twop; }

  if (twop==0)
    { int nb=numBits(Rg.m());
      nb=1<<(nb-1);
      if (nb==Rg.m())
        { //cout << Rg.m() << " " << PrD.pr << endl;
          root[0]=Find_Primitive_Root_2power(Rg.m(),PrD);
          Inv(root[1],root[0],PrD);
          to_modp(iphi,Rg.phi_m(),PrD);
          Inv(iphi,iphi,PrD);
        }
    }
  else 
    { bigint pr=PrD.pr;
      if ((pr-1)%(2*Rg.m())!=0)
	{ throw invalid_params(); }
      root[0]=Find_Primitive_Root_2m(Rg.m(),Rg.Phi(),PrD);
      Inv(root[1],root[0],PrD); 

      int ptwop=twop; if (twop<0) { ptwop=-twop; }

      powers.resize(2,vector<modp>(Rg.m()));
      powers_i.resize(2,vector<modp>(Rg.m()));
      b.resize(2,vector<modp>(ptwop));

      modp rInv,bi;
      bigint ee=ptwop; ee=ee*Rg.m();
      for (int r=0; r<2; r++)
        { assignOne(powers[r][0],PrD);
          if (r==0)
            { to_modp(powers_i[0][0],ptwop,PrD);  }
          else
            { to_modp(powers_i[1][0],ee,PrD);  }
          Inv(powers_i[r][0],powers_i[r][0],PrD);

          Inv(rInv,root[r],PrD);
          assignOne(b[r][Rg.m()-1],PrD);
          for (long i=1; i<Rg.m(); i++)
            { long iSqr=(i*i)%(2*Rg.m());
              Power(powers[r][i],root[r],iSqr,PrD);
              Mul(powers_i[r][i],powers[r][i],powers_i[r][0],PrD);
              Power(bi,rInv,iSqr,PrD);
              b[r][Rg.m()-1+i]=bi;
              b[r][Rg.m()-1-i]=bi;
            }
       }

      if (twop>0)
        { two_root.resize(2);
          two_root[0]=Find_Primitive_Root_2power(twop,PrD);
          Inv(two_root[1],two_root[0],PrD);

          for (int r=0; r<2; r++)
            { FFT_Iter(b[r],twop,two_root[0],PrD);  }
        }
    }
}
    

ostream& operator<<(ostream& s,const FFT_Data& FFTD)
{
  bigint ans;
  s << FFTD.prData << endl;
  s << FFTD.R << endl;

  to_bigint(ans,FFTD.root[0],FFTD.prData); s << ans << " ";
  to_bigint(ans,FFTD.root[1],FFTD.prData); s << ans << endl;

  s << FFTD.twop << endl;

  if (FFTD.twop==0)
    { to_bigint(ans,FFTD.iphi,FFTD.prData); s << ans << endl; }
  else if (FFTD.twop>0)
    { to_bigint(ans,FFTD.two_root[0],FFTD.prData); s << ans << " ";
      to_bigint(ans,FFTD.two_root[1],FFTD.prData); s << ans << endl;

      s << FFTD.powers[0].size() << endl;
      s << FFTD.powers_i[0].size() << endl;
      s << FFTD.b[0].size() << endl;
      for (int i=0; i<2; i++)
        { for (unsigned int j=0; j<FFTD.powers[i].size(); j++)
            { to_bigint(ans,FFTD.powers[i][j],FFTD.prData); s << ans << " "; }
          s << endl;
          for (unsigned int j=0; j<FFTD.powers_i[i].size(); j++)
            { to_bigint(ans,FFTD.powers_i[i][j],FFTD.prData); s << ans << " "; }
          s << endl;
          for (unsigned int j=0; j<FFTD.b[i].size(); j++)
            { to_bigint(ans,FFTD.b[i][j],FFTD.prData); s << ans << " "; }
          s << endl;
	}
    }
  return s;
}


istream& operator>>(istream& s,FFT_Data& FFTD)
{
  bigint ans;
  s >> FFTD.prData >> FFTD.R;
  FFTD.root.resize(2);
  s >> ans; to_modp(FFTD.root[0],ans,FFTD.prData);
  s >> ans; to_modp(FFTD.root[1],ans,FFTD.prData);

  s >> FFTD.twop;

  if (FFTD.twop==0)
    { s >> ans; to_modp(FFTD.iphi,ans,FFTD.prData); }
  else if (FFTD.twop>0)
    { FFTD.two_root.resize(2);

      s >> ans; to_modp(FFTD.two_root[0],ans,FFTD.prData);
      s >> ans; to_modp(FFTD.two_root[1],ans,FFTD.prData);

      int sz;
      s >> sz; FFTD.powers.resize(2, vector<modp>(sz));
      s >> sz; FFTD.powers_i.resize(2, vector<modp>(sz));
      s >> sz; FFTD.b.resize(2, vector<modp>(sz));
      for (int i=0; i<2; i++)
	{ for (unsigned j=0; j<FFTD.powers[i].size(); j++)
            { s >> ans; to_modp(FFTD.powers[i][j],ans,FFTD.prData); }
	  for (unsigned j=0; j<FFTD.powers_i[i].size(); j++)
            { s >> ans; to_modp(FFTD.powers_i[i][j],ans,FFTD.prData); }
	  for (unsigned j=0; j<FFTD.b[i].size(); j++)
            { s >> ans; to_modp(FFTD.b[i][j],ans,FFTD.prData); }
	}
    }
    
  return s;
}


void FFT_Data::pack(octetStream& o) const
{
  R.pack(o);
  prData.pack(o);
}


void FFT_Data::unpack(octetStream& o)
{
  R.unpack(o);
  prData.unpack(o);
  init(R, prData);
}


bool FFT_Data::operator!=(const FFT_Data& other) const
{
  if (R != other.R or prData != other.prData or root != other.root
      or twop != other.twop or two_root != other.two_root or b != other.b
      or iphi != other.iphi or powers != other.powers or powers_i != other.powers_i)
    {
      return true;
    }
  else
    return false;
}

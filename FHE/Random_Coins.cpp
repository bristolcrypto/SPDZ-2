// (C) 2018 University of Bristol. See License.txt


#include "Random_Coins.h"
#include "FHE_Keys.h"

Int_Random_Coins::Int_Random_Coins(const FHE_PK& pk) :
    Int_Random_Coins(pk.get_params())
{
}

Random_Coins::Random_Coins(const FHE_PK& pk) :
    Random_Coins(pk.get_params())
{
}

void add(Random_Coins& ans,const Random_Coins& x,const Random_Coins& y)
{ 
  if (x.params!=y.params)   { throw params_mismatch(); }
  if (ans.params!=y.params) { throw params_mismatch(); }

  add(ans.uu,x.uu,y.uu); 
  add(ans.vv,x.vv,y.vv); 
  add(ans.ww,x.ww,y.ww); 
}


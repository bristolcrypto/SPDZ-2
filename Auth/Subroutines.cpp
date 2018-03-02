// (C) 2018 University of Bristol. See License.txt


#include "Auth/Subroutines.h"

#include "Tools/random.h"
#include "Exceptions/Exceptions.h"
#include "Tools/Commit.h"

/* To ease readability as I re-write this program the following conventions
 * will be used.
 *   For a variable v index by a player i
 *   Comm_v[i] is the commitment string for player i
 *   Open_v[i] is the opening data for player i
 */


// Special version for octetStreams
void Commit(vector< vector<octetStream> >& Comm_data,
            vector<octetStream>& Open_data,
            const vector< vector<octetStream> >& data,const Player& P,int num_runs)
{
  int my_number=P.my_num();
  for (int i=0; i<num_runs; i++)
     { Comm_data[i].resize(P.num_players());
       Commit(Comm_data[i][my_number],Open_data[i],data[i][my_number],my_number);
       P.Broadcast_Receive(Comm_data[i]);
     }
}




// Special version for octetStreams
void Open(vector< vector<octetStream> >& data,
          const vector< vector<octetStream> >& Comm_data,
          const vector<octetStream>& My_Open_data,
          const Player& P,int num_runs,int dont)
{
  int my_number=P.my_num();
  int num_players=P.num_players();
  vector<octetStream> Open_data(num_players);
  for (int i=0; i<num_runs; i++)
     { if (i!=dont)
        { Open_data[my_number]=My_Open_data[i];
          P.Broadcast_Receive(Open_data);
          for (int j=0; j<num_players; j++) 
            { if (j!=my_number)
	        { if (!Open(data[i][j],Comm_data[i][j],Open_data[j],j))
	             { throw invalid_commitment(); }
	        }
	    }
        }
     }
}


void Open(vector< vector<octetStream> >& data,
          const vector< vector<octetStream> >& Comm_data,
          const vector<octetStream>& My_Open_data,
          const vector<int> open,
          const Player& P,int num_runs)
{
  int my_number=P.my_num();
  int num_players=P.num_players();
  vector<octetStream> Open_data(num_players);
  for (int i=0; i<num_runs; i++)
     { if (open[i]==1)
        { Open_data[my_number]=My_Open_data[i];
          P.Broadcast_Receive(Open_data);
          for (int j=0; j<num_players; j++) 
            { if (j!=my_number)
	         { if (!Open(data[i][j],Comm_data[i][j],Open_data[j],j))
	             { throw invalid_commitment(); }
	        }
	    }
        }
     }
}





void Commit_To_Challenge(vector<unsigned int>& e,
                         vector<octetStream>& Comm_e,vector<octetStream>& Open_e,
                         const Player& P,int num_runs)
{
  PRNG G;
  G.ReSeed();

  e.resize(P.num_players());
  Comm_e.resize(P.num_players());
  Open_e.resize(P.num_players());
  
  e[P.my_num()]=G.get_uint()%num_runs;
  octetStream ee; ee.store(e[P.my_num()]);
  Commit(Comm_e[P.my_num()],Open_e[P.my_num()],ee,P.my_num());
  P.Broadcast_Receive(Comm_e);
}


int Open_Challenge(vector<unsigned int>& e,vector<octetStream>& Open_e,
                   const vector<octetStream>& Comm_e,
                   const Player& P,int num_runs)
{
  // Now open the challenge commitments and determine which run was for real
  P.Broadcast_Receive(Open_e);
  
  int challenge=0;
  octetStream ee;
  for (int i = 0; i < P.num_players(); i++)
    { if (i != P.my_num())
        { if (!Open(ee,Comm_e[i],Open_e[i],i))
	     { throw invalid_commitment(); }
          ee.get(e[i]);
          }
      challenge+=e[i];
    }
  challenge = challenge % num_runs;

  return challenge;
}


template<class T>
void Create_Random(T& ans,const Player& P)
{
  PRNG G;
  G.ReSeed();
  vector<T> e(P.num_players());
  vector<octetStream> Comm_e(P.num_players());
  vector<octetStream> Open_e(P.num_players());
  
  e[P.my_num()].randomize(G);
  octetStream ee; 
  e[P.my_num()].pack(ee);
  Commit(Comm_e[P.my_num()],Open_e[P.my_num()],ee,P.my_num());
  P.Broadcast_Receive(Comm_e);
  
  P.Broadcast_Receive(Open_e);

  ans.assign_zero();
  for (int i = 0; i < P.num_players(); i++)
    { if (i != P.my_num())
        { if (!Open(ee,Comm_e[i],Open_e[i],i))
             { throw invalid_commitment(); }
          e[i].unpack(ee);
          }
      ans.add(ans,e[i]);
    }
}


void Create_Random_Seed(octet* seed,const Player& P,int len)
{
  PRNG G;
  G.ReSeed();
  vector<octetStream> e(P.num_players());
  vector<octetStream> Comm_e(P.num_players());
  vector<octetStream> Open_e(P.num_players());

  G.get_octetStream(e[P.my_num()],len);
  Commit(Comm_e[P.my_num()],Open_e[P.my_num()],e[P.my_num()],P.my_num());
  P.Broadcast_Receive(Comm_e);

  P.Broadcast_Receive(Open_e);

  memset(seed,0,len*sizeof(octet));
  for (int i = 0; i < P.num_players(); i++)
    { if (i != P.my_num())
        { if (!Open(e[i],Comm_e[i],Open_e[i],i))
             { throw invalid_commitment(); }
          }
      for (int j=0; j<len; j++)
	{ seed[j]=seed[j]^(e[i].get_data()[j]); }
    }
}


template<class T>
void Commit_And_Open(vector<T>& data,const Player& P)
{
  vector<octetStream> Comm_data(P.num_players());
  vector<octetStream> Open_data(P.num_players());

  octetStream ee;
  data[P.my_num()].pack(ee);
  Commit(Comm_data[P.my_num()],Open_data[P.my_num()],ee,P.my_num());
  P.Broadcast_Receive(Comm_data);

  P.Broadcast_Receive(Open_data);

  for (int i = 0; i < P.num_players(); i++)
    { if (i != P.my_num())
        { if (!Open(ee,Comm_data[i],Open_data[i],i))
             { throw invalid_commitment(); }
          data[i].unpack(ee);
        }
    }
}


void Commit_To_Seeds(vector<PRNG>& G,
                     vector< vector<octetStream> >& seeds,
                     vector< vector<octetStream> >& Comm_seeds,
		     vector<octetStream>& Open_seeds,
		     const Player& P,int num_runs)
{
  seeds.resize(num_runs);
  Comm_seeds.resize(num_runs);
  Open_seeds.resize(num_runs);
  for (int i=0; i<num_runs; i++)
    { G[i].ReSeed();
      seeds[i].resize(P.num_players());
      Comm_seeds[i].resize(P.num_players());
      Open_seeds[i].resize(P.num_players());
      seeds[i][P.my_num()].reset_write_head();
      seeds[i][P.my_num()].append(G[i].get_seed(),SEED_SIZE);
    }
  Commit(Comm_seeds,Open_seeds,seeds,P,num_runs);
}


void generate_challenge(vector<int>& challenge, const Player& P)
{
  octet seed[SEED_SIZE];
  Create_Random_Seed(seed, P, SEED_SIZE);
  PRNG G;
  G.SetSeed(seed);
  for (size_t i = 0; i < challenge.size(); i++)
    challenge[i] = G.get_uchar() % 2;
}



template void Commit_And_Open(vector<gf2n>& data,const Player& P);
template void Create_Random(gf2n& ans,const Player& P);

#ifdef USE_GF2N_LONG
template void Commit_And_Open(vector<gf2n_short>& data,const Player& P);
template void Create_Random(gf2n_short& ans,const Player& P);
#endif

template void Commit_And_Open(vector<gfp>& data,const Player& P);
template void Create_Random(gfp& ans,const Player& P);

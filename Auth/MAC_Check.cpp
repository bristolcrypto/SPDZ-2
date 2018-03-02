// (C) 2018 University of Bristol. See License.txt


#include "Auth/MAC_Check.h"
#include "Auth/Subroutines.h"
#include "Exceptions/Exceptions.h"

#include "Tools/random.h"
#include "Tools/time-func.h"
#include "Tools/int.h"

#include "Math/gfp.h"
#include "Math/gf2n.h"

#include <algorithm>

const char* mc_timer_names[] = {
        "sending",
        "receiving and adding",
        "broadcasting",
        "receiving summed values",
        "random seed",
        "commit and open",
        "wait for summer thread",
        "receiving",
        "summing",
        "waiting for select()"
};


template<class T>
MAC_Check<T>::MAC_Check(const T& ai, int opening_sum, int max_broadcast, int send_player) :
    TreeSum<T>(opening_sum, max_broadcast, send_player)
{
  popen_cnt=0;
  alphai=ai;
  values_opened=0;
}

template<class T>
MAC_Check<T>::~MAC_Check()
{
}

template<class T>
void MAC_Check<T>::POpen_Begin(vector<T>& values,const vector<Share<T> >& S,const Player& P)
{
  AddToMacs(S);

  values.resize(S.size());
  for (unsigned int i=0; i<S.size(); i++)
    { values[i]=S[i].get_share(); }

  this->start(values, P);

  values_opened += S.size();
}

template<class T>
void MAC_Check<T>::POpen_End(vector<T>& values,const vector<Share<T> >& S,const Player& P)
{
  S.size();

  this->finish(values, P);

  popen_cnt += values.size();
  CheckIfNeeded(P);

  /* not compatible with continuous communication
  send_player++;
  if (send_player==P.num_players())
    { send_player=0; }
  */
}


template<class T>
void MAC_Check<T>::AddToMacs(const vector<Share<T> >& shares)
{
  for (unsigned int i = 0; i < shares.size(); i++)
    macs.push_back(shares[i].get_mac());
}


template<class T>
void MAC_Check<T>::AddToValues(vector<T>& values)
{
  vals.insert(vals.end(), values.begin(), values.end());
}


template<class T>
void MAC_Check<T>::GetValues(vector<T>& values)
{
  int size = values.size();
  if (popen_cnt + size > int(vals.size()))
    {
      stringstream ss;
      ss << "wanted " << values.size() << " values from " << popen_cnt << ", only " << vals.size() << " in store";
      throw out_of_range(ss.str());
    }
  values.clear();
  typename vector<T>::iterator first = vals.begin() + popen_cnt;
  values.insert(values.end(), first, first + size);
}


template<class T>
void MAC_Check<T>::CheckIfNeeded(const Player& P)
{
  if (WaitingForCheck() >= POPEN_MAX)
    Check(P);
}


template <class T>
void MAC_Check<T>::AddToCheck(const T& mac, const T& value, const Player& P)
{
  macs.push_back(mac);
  vals.push_back(value);
  popen_cnt++;
  CheckIfNeeded(P);
}



template<class T>
void MAC_Check<T>::Check(const Player& P)
{
  if (WaitingForCheck() == 0)
    return;

  //cerr << "In MAC Check : " << popen_cnt << endl;
  octet seed[SEED_SIZE];
  this->timers[SEED].start();
  Create_Random_Seed(seed,P,SEED_SIZE);
  this->timers[SEED].stop();
  PRNG G;
  G.SetSeed(seed);

  Share<T>  sj;
  T a,gami,h,temp;
  a.assign_zero();
  gami.assign_zero();
  vector<T> tau(P.num_players());
  for (int i=0; i<popen_cnt; i++)
    { h.almost_randomize(G);
      temp.mul(h,vals[i]);
      a.add(a,temp);
      
      temp.mul(h,macs[i]);
      gami.add(gami,temp);
    }
  vals.erase(vals.begin(), vals.begin() + popen_cnt);
  macs.erase(macs.begin(), macs.begin() + popen_cnt);
  temp.mul(alphai,a);
  tau[P.my_num()].sub(gami,temp);

  //cerr << "\tCommit and Open" << endl;
  this->timers[COMMIT].start();
  Commit_And_Open(tau,P);
  this->timers[COMMIT].stop();

  //cerr << "\tFinal Check" << endl;

  T t;
  t.assign_zero();
  for (int i=0; i<P.num_players(); i++)
    { t.add(t,tau[i]); }
  if (!t.is_zero()) { throw mac_fail(); }

  popen_cnt=0;
}

template<class T>
int mc_base_id(int function_id, int thread_num)
{
  return (function_id << 28) + ((T::field_type() + 1) << 24) + (thread_num << 16);
}

template<class T>
Separate_MAC_Check<T>::Separate_MAC_Check(const T& ai, Names& Nms,
    int thread_num, int opening_sum, int max_broadcast, int send_player) :
    MAC_Check<T>(ai, opening_sum, max_broadcast, send_player),
    check_player(Nms, mc_base_id<T>(1, thread_num))
{
}

template<class T>
void Separate_MAC_Check<T>::Check(const Player& P)
{
  P.my_num();
  MAC_Check<T>::Check(check_player);
}


template<class T>
void* run_summer_thread(void* summer)
{
  ((Summer<T>*) summer)->run();
  return 0;
}

template <class T>
Parallel_MAC_Check<T>::Parallel_MAC_Check(const T& ai, Names& Nms,
    int thread_num, int opening_sum, int max_broadcast, int base_player) :
    Separate_MAC_Check<T>(ai, Nms, thread_num, opening_sum, max_broadcast, base_player),
    send_player(Nms, mc_base_id<T>(2, thread_num)),
    send_base_player(base_player)
{
  int sum_players = Nms.num_players();
  Player* summer_send_player = &send_player;
  for (int i = 0; ; i++)
    {
      int last_sum_players = sum_players;
      sum_players = (sum_players - 2 + opening_sum) / opening_sum;
      int next_sum_players = (sum_players - 2 + opening_sum) / opening_sum;
      if (sum_players == 0)
        break;
      Player* summer_receive_player = summer_send_player;
      summer_send_player = new Player(Nms, mc_base_id<T>(3, thread_num));
      summers.push_back(new Summer<T>(sum_players, last_sum_players, next_sum_players,
              summer_send_player, summer_receive_player, *this));
      pthread_create(&(summers[i]->thread), 0, run_summer_thread<T>, summers[i]);
    }
  receive_player = summer_send_player;
}

template<class T>
Parallel_MAC_Check<T>::~Parallel_MAC_Check()
{
  for (unsigned int i = 0; i < summers.size(); i++)
    {
      summers[i]->input_queue.stop();
      pthread_join(summers[i]->thread, 0);
      delete summers[i];
    }
}

template<class T>
void Parallel_MAC_Check<T>::POpen_Begin(vector<T>& values,
        const vector<Share<T> >& S, const Player& P)
{
  values.size();
  this->AddToMacs(S);

  int my_relative_num = positive_modulo(P.my_num() - send_base_player, P.num_players());
  int sum_players = (P.num_players() - 2 + this->opening_sum) / this->opening_sum;
  int receiver = positive_modulo(send_base_player + my_relative_num % sum_players, P.num_players());

  // use queue rather sending to myself
  if (receiver == P.my_num())
    {
      for (unsigned int i = 0; i < S.size(); i++)
        values[i] = S[i].get_share();
      summers.front()->share_queue.push(values);
    }
  else
    {
      this->os.reset_write_head();
      for (unsigned int i=0; i<S.size(); i++)
          S[i].get_share().pack(this->os);
      this->timers[SEND].start();
      send_player.send_to(receiver,this->os,true);
      this->timers[SEND].stop();
    }

  for (unsigned int i = 0; i < summers.size(); i++)
      summers[i]->input_queue.push(S.size());

  this->values_opened += S.size();
  send_base_player = (send_base_player + 1) % send_player.num_players();
}

template<class T>
void Parallel_MAC_Check<T>::POpen_End(vector<T>& values,
        const vector<Share<T> >& S, const Player& P)
{
  int last_size = 0;
  this->timers[WAIT_SUMMER].start();
  summers.back()->output_queue.pop(last_size);
  this->timers[WAIT_SUMMER].stop();
  if (int(values.size()) != last_size)
    {
      stringstream ss;
      ss << "stopopen wants " << values.size() << " values, but I have " << last_size << endl;
      throw Processor_Error(ss.str().c_str());
    }

  if (this->base_player == P.my_num())
    {
      value_queue.pop(values);
      if (int(values.size()) != last_size)
        throw Processor_Error("wrong number of local values");
      else
        this->AddToValues(values);
    }
  this->MAC_Check<T>::POpen_End(values, S, *receive_player);
  this->base_player = (this->base_player + 1) % send_player.num_players();
}



template<class T>
Direct_MAC_Check<T>::Direct_MAC_Check(const T& ai, Names& Nms, int num) : Separate_MAC_Check<T>(ai, Nms, num) {
  open_counter = 0;
}


template<class T>
Direct_MAC_Check<T>::~Direct_MAC_Check() {
  cerr << T::type_string() << " open counter: " << open_counter << endl;
}


template<class T>
void Direct_MAC_Check<T>::POpen_Begin(vector<T>& values,const vector<Share<T> >& S,const Player& P)
{
  values.resize(S.size());
  this->os.reset_write_head();
  for (unsigned int i=0; i<S.size(); i++)
    S[i].get_share().pack(this->os);
  this->timers[SEND].start();
  P.send_all(this->os,true);
  this->timers[SEND].stop();

  this->AddToMacs(S);
  for (unsigned int i=0; i<S.size(); i++)
    this->vals.push_back(S[i].get_share());
}

template<class T, int t>
void direct_add_openings(vector<T>& values, const Player& P, vector<octetStream>& os)
{
  for (unsigned int i=0; i<values.size(); i++)
    for (int j=0; j<P.num_players(); j++)
      if (j!=P.my_num())
	values[i].template add<t>(os[j].consume(T::size()));
}

template<class T>
void Direct_MAC_Check<T>::POpen_End(vector<T>& values,const vector<Share<T> >& S,const Player& P)
{
  S.size();
  oss.resize(P.num_players());
  this->GetValues(values);

  this->timers[RECV].start();

  for (int j=0; j<P.num_players(); j++)
    if (j!=P.my_num())
      P.receive_player(j,oss[j],true);

  this->timers[RECV].stop();
  open_counter++;

  if (T::t() == 2)
    direct_add_openings<T,2>(values, P, oss);
  else
    direct_add_openings<T,0>(values, P, oss);

  for (unsigned int i = 0; i < values.size(); i++)
    this->vals[this->popen_cnt+i] = values[i];

  this->popen_cnt += values.size();
  this->CheckIfNeeded(P);
}

template class MAC_Check<gfp>;
template class Direct_MAC_Check<gfp>;
template class Parallel_MAC_Check<gfp>;

template class MAC_Check<gf2n>;
template class Direct_MAC_Check<gf2n>;
template class Parallel_MAC_Check<gf2n>;

#ifdef USE_GF2N_LONG
template class MAC_Check<gf2n_short>;
template class Direct_MAC_Check<gf2n_short>;
template class Parallel_MAC_Check<gf2n_short>;
#endif

// (C) 2017 University of Bristol. See License.txt


#include "Auth/MAC_Check.h"
#include "Auth/Subroutines.h"
#include "Exceptions/Exceptions.h"

#include "Tools/random.h"
#include "Tools/time-func.h"
#include "Tools/int.h"

#include "Math/gfp.h"
#include "Math/gf2n.h"

#include <algorithm>

enum mc_timer { SEND, RECV_ADD, BCAST, RECV_SUM, SEED, COMMIT, WAIT_SUMMER, RECV, SUM, SELECT, MAX_TIMER };
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
    base_player(send_player), opening_sum(opening_sum), max_broadcast(max_broadcast)
{
  popen_cnt=0;
  alphai=ai;
  values_opened=0;
  timers.resize(MAX_TIMER);
}

template<class T>
MAC_Check<T>::~MAC_Check()
{
  for (unsigned int i = 0; i < timers.size(); i++)
    if (timers[i].elapsed() > 0)
      cerr << T::type_string() << " " << mc_timer_names[i] << ": "
        << timers[i].elapsed() << endl;

  for (unsigned int i = 0; i < player_timers.size(); i++)
    if (player_timers[i].elapsed() > 0)
      cerr << T::type_string() << " waiting for " << i << ": "
        << player_timers[i].elapsed() << endl;
}

template<class T, int t>
void add_openings(vector<T>& values, const Player& P, int sum_players, int last_sum_players, int send_player, MAC_Check<T>& MC)
{
  MC.player_timers.resize(P.num_players());
  vector<octetStream>& oss = MC.oss;
  oss.resize(P.num_players());
  vector<int> senders;
  senders.reserve(P.num_players());

  for (int relative_sender = positive_modulo(P.my_num() - send_player, P.num_players()) + sum_players;
      relative_sender < last_sum_players; relative_sender += sum_players)
    {
      int sender = positive_modulo(send_player + relative_sender, P.num_players());
      senders.push_back(sender);
    }

  for (int j = 0; j < (int)senders.size(); j++)
    P.request_receive(senders[j], oss[j]);

  for (int j = 0; j < (int)senders.size(); j++)
    {
      int sender = senders[j];
      MC.player_timers[sender].start();
      P.wait_receive(sender, oss[j], true);
      MC.player_timers[sender].stop();
      if ((unsigned)oss[j].get_length() < values.size() * T::size())
        {
          stringstream ss;
          ss << "Not enough information received, expected "
              << values.size() * T::size() << " bytes, got "
              << oss[j].get_length();
          throw Processor_Error(ss.str());
        }
      MC.timers[SUM].start();
      for (unsigned int i=0; i<values.size(); i++)
        {
          values[i].template add<t>(oss[j].consume(T::size()));
        }
      MC.timers[SUM].stop();
    }
}

template<class T>
void MAC_Check<T>::POpen_Begin(vector<T>& values,const vector<Share<T> >& S,const Player& P)
{
  AddToMacs(S);

  for (unsigned int i=0; i<S.size(); i++)
    { values[i]=S[i].get_share(); }

  os.reset_write_head();
  int sum_players = P.num_players();
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  while (true)
    {
      int last_sum_players = sum_players;
      sum_players = (sum_players - 2 + opening_sum) / opening_sum;
      if (sum_players == 0)
        break;
      if (my_relative_num >= sum_players && my_relative_num < last_sum_players)
        {
          for (unsigned int i=0; i<S.size(); i++)
            { values[i].pack(os); }
          int receiver = positive_modulo(base_player + my_relative_num % sum_players, P.num_players());
          timers[SEND].start();
          P.send_to(receiver,os,true);
          timers[SEND].stop();
        }

      if (my_relative_num < sum_players)
        {
          timers[RECV_ADD].start();
          if (T::t() == 2)
            add_openings<T,2>(values, P, sum_players, last_sum_players, base_player, *this);
          else
            add_openings<T,0>(values, P, sum_players, last_sum_players, base_player, *this);
          timers[RECV_ADD].stop();
        }
    }

  if (P.my_num() == base_player)
    {
      os.reset_write_head();
      for (unsigned int i=0; i<S.size(); i++)
        { values[i].pack(os); }
      timers[BCAST].start();
      for (int i = 1; i < max_broadcast && i < P.num_players(); i++)
        {
          P.send_to((base_player + i) % P.num_players(), os, true);
        }
      timers[BCAST].stop();
      AddToValues(values);
    }
  else if (my_relative_num * max_broadcast < P.num_players())
    {
      int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
      ReceiveValues(values, P, sender);
      timers[BCAST].start();
      for (int i = 0; i < max_broadcast; i++)
        {
          int relative_receiver = (my_relative_num * max_broadcast + i);
          if (relative_receiver < P.num_players())
            {
              int receiver = (base_player + relative_receiver) % P.num_players();
              P.send_to(receiver, os, true);
            }
        }
      timers[BCAST].stop();
    }

  values_opened += S.size();
}


template<class T>
void MAC_Check<T>::POpen_End(vector<T>& values,const vector<Share<T> >& S,const Player& P)
{
  S.size();
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  if (my_relative_num * max_broadcast >= P.num_players())
    {
      int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
      ReceiveValues(values, P, sender);
    }
  else
    GetValues(values);

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
void MAC_Check<T>::ReceiveValues(vector<T>& values, const Player& P, int sender)
{
  timers[RECV_SUM].start();
  P.receive_player(sender, os, true);
  timers[RECV_SUM].stop();
  for (unsigned int i = 0; i < values.size(); i++)
    values[i].unpack(os);
  AddToValues(values);
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
  timers[SEED].start();
  Create_Random_Seed(seed,P,SEED_SIZE);
  timers[SEED].stop();
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
  timers[COMMIT].start();
  Commit_And_Open(tau,P);
  timers[COMMIT].stop();

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
  values.size();
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

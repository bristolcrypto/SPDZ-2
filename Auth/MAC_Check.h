// (C) 2018 University of Bristol. See License.txt

#ifndef _MAC_Check
#define _MAC_Check

/* Class for storing MAC Check data and doing the Check */

#include <vector>
#include <deque>
using namespace std;

#include "Math/Share.h"
#include "Networking/Player.h"
#include "Networking/ServerSocket.h"
#include "Auth/Summer.h"
#include "Tools/time-func.h"


/* The MAX number of things we will partially open before running
 * a MAC Check
 *
 * Keep this at much less than 1MB of data to be able to cope with
 * multi-threaded players
 *
 */
#define POPEN_MAX 1000000


template<class T>
class TreeSum
{
protected:
  int base_player;
  int opening_sum;
  int max_broadcast;
  octetStream os;

  void ReceiveValues(vector<T>& values, const Player& P, int sender);
  virtual void AddToValues(vector<T>& values) { (void)values; }
  virtual void GetValues(vector<T>& values) { (void)values; }

public:
  vector<octetStream> oss;
  vector<Timer> timers;
  vector<Timer> player_timers;

  TreeSum(int opening_sum = 10, int max_broadcast = 10, int base_player = 0);
  virtual ~TreeSum();

  void start(vector<T>& values, const Player& P);
  void finish(vector<T>& values, const Player& P);
  void run(vector<T>& values, const Player& P);

  octetStream& get_buffer() { return os; }

  size_t report_size(ReportType type);
};


template<class T>
class MAC_Check : public TreeSum<T>
{
  protected:

  /* POpen Data */
  int popen_cnt;
  vector<T> macs;
  vector<T> vals;

  /* MAC Share */
  T alphai;

  void AddToMacs(const vector< Share<T> >& shares);
  void AddToValues(vector<T>& values);
  void GetValues(vector<T>& values);
  void CheckIfNeeded(const Player& P);
  int WaitingForCheck()
    { return max(macs.size(), vals.size()); }

  public:

  int values_opened;

  MAC_Check(const T& ai, int opening_sum=10, int max_broadcast=10, int send_player=0);
  virtual ~MAC_Check();

  /* Run protocols to partially open data and check the MACs are 
   * all OK.
   *  - Implicit assume that the amount of data being sent does
   *    not overload the OS
   * Begin and End expect the same arrays values and S passed to them
   * and they expect values to be of the same size as S.
   */
  virtual void POpen_Begin(vector<T>& values,const vector<Share<T> >& S,const Player& P);
  virtual void POpen_End(vector<T>& values,const vector<Share<T> >& S,const Player& P);
  void AddToCheck(const T& mac, const T& value, const Player& P);
  virtual void Check(const Player& P);

  int number() const { return values_opened; }

  const T& get_alphai() const { return alphai; }
};


template<class T, int t>
  void add_openings(vector<T>& values, const Player& P, int sum_players, int last_sum_players, int send_player, TreeSum<T>& MC);


template<class T>
class Separate_MAC_Check: public MAC_Check<T>
{
  // Different channel for checks
  Player check_player;

protected:
  // No sense to expose this
  Separate_MAC_Check(const T& ai, Names& Nms, int thread_num, int opening_sum=10, int max_broadcast=10, int send_player=0);
  virtual ~Separate_MAC_Check() {};

public:
  virtual void Check(const Player& P);
};


template<class T>
class Parallel_MAC_Check: public Separate_MAC_Check<T>
{
  // Different channel for every round
  Player send_player;
  // Managed by Summer
  Player* receive_player;

  vector< Summer<T>* > summers;

  int send_base_player;

  WaitQueue< vector<T> > value_queue;

public:
  Parallel_MAC_Check(const T& ai, Names& Nms, int thread_num, int opening_sum=10, int max_broadcast=10, int send_player=0);
  virtual ~Parallel_MAC_Check();

  virtual void POpen_Begin(vector<T>& values,const vector<Share<T> >& S,const Player& P);
  virtual void POpen_End(vector<T>& values,const vector<Share<T> >& S,const Player& P);

  friend class Summer<T>;
};


template<class T>
class Direct_MAC_Check: public Separate_MAC_Check<T>
{
  int open_counter;
  vector<octetStream> oss;

public:
  Direct_MAC_Check(const T& ai, Names& Nms, int thread_num);
  ~Direct_MAC_Check();

  void POpen_Begin(vector<T>& values,const vector<Share<T> >& S,const Player& P);
  void POpen_End(vector<T>& values,const vector<Share<T> >& S,const Player& P);
};


enum mc_timer { SEND, RECV_ADD, BCAST, RECV_SUM, SEED, COMMIT, WAIT_SUMMER, RECV, SUM, SELECT, MAX_TIMER };

template<class T>
TreeSum<T>::TreeSum(int opening_sum, int max_broadcast, int base_player) :
    base_player(base_player), opening_sum(opening_sum), max_broadcast(max_broadcast)
{
  timers.resize(MAX_TIMER);
}

template<class T>
TreeSum<T>::~TreeSum()
{
#ifdef TREESUM_TIMINGS
  for (unsigned int i = 0; i < timers.size(); i++)
    if (timers[i].elapsed() > 0)
      cerr << T::type_string() << " " << mc_timer_names[i] << ": "
        << timers[i].elapsed() << endl;

  for (unsigned int i = 0; i < player_timers.size(); i++)
    if (player_timers[i].elapsed() > 0)
      cerr << T::type_string() << " waiting for " << i << ": "
        << player_timers[i].elapsed() << endl;
#endif
}

template<class T>
void TreeSum<T>::run(vector<T>& values, const Player& P)
{
  start(values, P);
  finish(values, P);
}

template<class T>
size_t TreeSum<T>::report_size(ReportType type)
{
  if (type == CAPACITY)
    return os.get_max_length();
  else
    return os.get_length();
}

template<class T, int t>
void add_openings(vector<T>& values, const Player& P, int sum_players, int last_sum_players, int send_player, TreeSum<T>& MC)
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
          values[i].template add<t>(oss[j]);
        }
      MC.timers[SUM].stop();
    }
}

template<class T>
void TreeSum<T>::start(vector<T>& values, const Player& P)
{
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
          for (unsigned int i=0; i<values.size(); i++)
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
      for (unsigned int i=0; i<values.size(); i++)
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
}

template<class T>
void TreeSum<T>::finish(vector<T>& values, const Player& P)
{
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  if (my_relative_num * max_broadcast >= P.num_players())
    {
      int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
      ReceiveValues(values, P, sender);
    }
  else
    GetValues(values);
}

template<class T>
void TreeSum<T>::ReceiveValues(vector<T>& values, const Player& P, int sender)
{
  timers[RECV_SUM].start();
  P.receive_player(sender, os, true);
  timers[RECV_SUM].stop();
  for (unsigned int i = 0; i < values.size(); i++)
    values[i].unpack(os);
  AddToValues(values);
}

#endif

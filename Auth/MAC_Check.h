// (C) 2017 University of Bristol. See License.txt

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
class MAC_Check
{
  protected:

  /* POpen Data */
  int popen_cnt;
  vector<T> macs;
  vector<T> vals;
  int base_player;
  int opening_sum;
  int max_broadcast;
  octetStream os;

  /* MAC Share */
  T alphai;

  void AddToMacs(const vector< Share<T> >& shares);
  void AddToValues(vector<T>& values);
  void ReceiveValues(vector<T>& values, const Player& P, int sender);
  void GetValues(vector<T>& values);
  void CheckIfNeeded(const Player& P);
  int WaitingForCheck()
    { return max(macs.size(), vals.size()); }

  public:

  int values_opened;
  vector<Timer> timers;
  vector<Timer> player_timers;
  vector<octetStream> oss;

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
  void add_openings(vector<T>& values, const Player& P, int sum_players, int last_sum_players, int send_player, MAC_Check<T>& MC);


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

#endif

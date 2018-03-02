// (C) 2018 University of Bristol. See License.txt

/*
 * Summer.h
 *
 */

#ifndef OFFLINE_SUMMER_H_
#define OFFLINE_SUMMER_H_

#include "Networking/Player.h"
#include "Tools/WaitQueue.h"
#include "Tools/time-func.h"

#include <pthread.h>
#include <vector>
using namespace std;

template<class T>
class Parallel_MAC_Check;

template<class T>
class Summer
{
    int sum_players, last_sum_players, next_sum_players;
    int base_player;
    Parallel_MAC_Check<T>& MC;
    Player* send_player;
    Player* receive_player;
    Timer timer;

public:
    vector<T> values;

    pthread_t thread;
    WaitQueue<int> input_queue, output_queue;
    bool stop;
    int size;
    WaitQueue< vector<T> > share_queue;

    Summer(int sum_players, int last_sum_players, int next_sum_players,
            Player* send_player, Player* receive_player, Parallel_MAC_Check<T>& MC);
    ~Summer();
    void run();
};

#endif /* OFFLINE_SUMMER_H_ */

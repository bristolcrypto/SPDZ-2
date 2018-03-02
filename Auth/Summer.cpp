// (C) 2018 University of Bristol. See License.txt

/*
 * Summer.cpp
 *
 */

#include "Auth/Summer.h"
#include "Auth/MAC_Check.h"
#include "Tools/int.h"

template<class T>
Summer<T>::Summer(int sum_players, int last_sum_players, int next_sum_players,
        Player* send_player, Player* receive_player, Parallel_MAC_Check<T>& MC) :
        sum_players(sum_players), last_sum_players(last_sum_players), next_sum_players(next_sum_players),
        base_player(0), MC(MC),send_player(send_player), receive_player(receive_player),
        thread(0), stop(false), size(0)
{
    cout << "Setting up summation by " << sum_players << " players" << endl;
}

template<class T>
Summer<T>::~Summer()
{
    delete send_player;
    if (timer.elapsed())
    cout << T::type_string() << " summation by " << sum_players << " players: "
        << timer.elapsed() << endl;
}

template<class T>
void Summer<T>::run()
{
    octetStream os;

    while (true)
    {
        int size = 0;
        if (!input_queue.pop(size))
            break;

        int my_relative_num = positive_modulo(send_player->my_num() - base_player, send_player->num_players());
        if (my_relative_num < sum_players)
        {
            // first summer takes inputs from queue
            if (last_sum_players == send_player->num_players())
                share_queue.pop(values);
            else
            {
                values.resize(size);
                receive_player->receive_player(receive_player->my_num(),os,true);
                for (int i = 0; i < size; i++)
                    values[i].unpack(os);
            }

            timer.start();
            if (T::t() == 2)
                add_openings<T,2>(values, *receive_player, sum_players,
                        last_sum_players, base_player, MC);
            else
                add_openings<T,0>(values, *receive_player, sum_players,
                        last_sum_players, base_player, MC);
            timer.stop();

            os.reset_write_head();
            for (int i = 0; i < size; i++)
                values[i].pack(os);

            if (sum_players > 1)
            {
                int receiver = positive_modulo(base_player + my_relative_num % next_sum_players,
                        send_player->num_players());
                send_player->send_to(receiver, os, true);
            }
            else
            {
                send_player->send_all(os);
                MC.value_queue.push(values);
            }
        }

        if (sum_players == 1)
            output_queue.push(size);

        base_player = (base_player + 1) % send_player->num_players();
    }
}

template class Summer<gfp>;
template class Summer<gf2n>;

#ifdef USE_GF2N_LONG
template class Summer<gf2n_short>;
#endif

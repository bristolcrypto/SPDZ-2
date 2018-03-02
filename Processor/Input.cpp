// (C) 2018 University of Bristol. See License.txt

/*
 * Input.cpp
 *
 */

#include "Input.h"
#include "Processor.h"

template<class T>
Input<T>::Input(Processor& proc, MAC_Check<T>& mc) :
        proc(proc), MC(mc), shares(proc.P.num_players()), values_input(0)
{
    buffer.setup(&proc.private_input, -1, proc.private_input_filename);
}

template<class T>
Input<T>::~Input()
{
    if (timer.elapsed() > 0)
        cerr << T::type_string() << " inputs: " << timer.elapsed() << endl;
}

template<class T>
void Input<T>::adjust_mac(Share<T>& share, T& value)
{
    T tmp;
    tmp.mul(MC.get_alphai(), value);
    tmp.add(share.get_mac(),tmp);
    share.set_mac(tmp);
}

template<class T>
void Input<T>::start(int player, int n_inputs)
{
    shares[player].resize(n_inputs);
    vector<T> rr(n_inputs);

    if (player == proc.P.my_num())
    {
        octetStream o;

        for (int i = 0; i < n_inputs; i++)
        {
            T rr, t;
            Share<T>& share = shares[player][i];
            proc.DataF.get_input(share, rr, player);
            T xi;
            try
            {
                buffer.input(t);
            }
            catch (not_enough_to_buffer& e)
            {
                throw runtime_error("Insufficient input data to buffer");
            }
            t.sub(t, rr);
            t.pack(o);
            xi.add(t, share.get_share());
            share.set_share(xi);
            adjust_mac(share, t);
        }

        proc.P.send_all(o, true);
        values_input += n_inputs;
    }
    else
    {
        T t;
        for (int i = 0; i < n_inputs; i++)
            proc.DataF.get_input(shares[player][i], t, player);
    }
}

template<class T>
void Input<T>::stop(int player, vector<int> targets)
{
    for (unsigned int i = 0; i < targets.size(); i++)
        proc.get_S_ref<T>(targets[i]) = shares[player][i];

    if (proc.P.my_num() != player)
    {
        T t;
        octetStream o;
        timer.start();
        proc.P.receive_player(player, o, true);
        timer.stop();
        for (unsigned int i = 0; i < targets.size(); i++)
        {
            Share<T>& share = proc.get_S_ref<T>(targets[i]);
            t.unpack(o);
            adjust_mac(share, t);
        }
    }
}

template class Input<gf2n>;
template class Input<gfp>;

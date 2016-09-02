// (C) 2016 University of Bristol. See License.txt

/*
 * Input.cpp
 *
 */

#include "Input.h"
#include "Processor.h"

template<class T>
Input<T>::Input(Processor& proc, MAC_Check<T>& mc) :
        proc(proc), MC(mc), values_input(0)
{
    buffer.setup(&proc.private_input, -1, "private input");
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
    if (player == proc.P.my_num())
    {
        octetStream o;
        shares.resize(n_inputs);

        for (int i = 0; i < n_inputs; i++)
        {
            T rr, t;
            Share<T>& share = shares[i];
            proc.DataF.get_input(share, rr, player);
            T xi;
            buffer.input(t);
            t.sub(t, rr);
            t.pack(o);
            xi.add(t, share.get_share());
            share.set_share(xi);
            adjust_mac(share, t);
        }

        proc.P.send_all(o, true);
        values_input += n_inputs;
    }
}

template<class T>
void Input<T>::stop(int player, vector<int> targets)
{
    T tmp;

    if (player == proc.P.my_num())
    {
        for (unsigned int i = 0; i < targets.size(); i++)
            proc.get_S_ref<T>(targets[i]) = shares[i];
    }
    else
    {
        T t;
        octetStream o;
        timer.start();
        proc.P.receive_player(player, o, true);
        timer.stop();
        for (unsigned int i = 0; i < targets.size(); i++)
        {
            Share<T>& share = proc.get_S_ref<T>(targets[i]);
            proc.DataF.get_input(share, t, player);
            t.unpack(o);
            adjust_mac(share, t);
        }
    }
}

template class Input<gf2n>;
template class Input<gfp>;

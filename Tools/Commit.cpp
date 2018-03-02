// (C) 2018 University of Bristol. See License.txt


#include "Commit.h"
#include "random.h"
#include "int.h"

void Commit(octetStream& comm,octetStream& open,const octetStream& message, int send_player)
{
    open.store(send_player);
    open.append(message.get_data(), message.get_length());
    open.append_random(SEED_SIZE);
    comm = open.hash();
}

bool Open(octetStream& message,const octetStream& comm,const octetStream& open, int send_player)
{
    octetStream h = open.hash();
    octet* open_bytes = open.get_data();
    // first 4 bytes are player no.
    int open_player = BYTES_TO_INT(open_bytes);
    if (!(h.equals(comm) && open_player == send_player))
    {
        return false;
    }
    message.reset_write_head();
    message.append(open_bytes + sizeof(int), open.get_length() - SEED_SIZE - sizeof(int));
    return true;
}

void Commitment::commit(const octetStream& message)
{
    open.reset_write_head();
    open.append_random(SEED_SIZE);
    commit(message, open);
}

void Commitment::commit(const octetStream& message, const octetStream& open)
{
    SHA1 hash;
    hash.update(&send_player, sizeof(send_player));
    hash.update(message);
    hash.update(open);
    hash.final(comm);
}

void Commitment::check(const octetStream& message, const octetStream& comm,
        const octetStream& open)
{
    commit(message, open);
    if (!(comm == this->comm))
        throw invalid_commitment();
}

void AllCommitments::commit_and_open(const octetStream& message)
{
    Commitment mine(P.my_num());
    mine.commit(message);
    comms[P.my_num()] = mine.comm;
    opens[P.my_num()] = mine.open;
    P.Broadcast_Receive(comms);
    P.Broadcast_Receive(opens);
}

void AllCommitments::check(int player, const octetStream& message)
{
    Commitment(player).check(message, comms[player], opens[player]);
}

void AllCommitments::check_relative(int diff, const octetStream& message)
{
    check((P.my_num() + P.num_players() - diff) % P.num_players(), message);
}

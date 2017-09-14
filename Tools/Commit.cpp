// (C) 2017 University of Bristol. See License.txt


#include "Commit.h"
#include "random.h"

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

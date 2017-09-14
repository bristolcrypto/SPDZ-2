// (C) 2017 University of Bristol. See License.txt

#ifndef _Commit
#define _Commit

/* Define the hash based commitment scheme */

#include "Tools/octetStream.h"

/*
 * Commit using comm = hash(send_player || message || r)
 * where r is SEED_SIZE random bytes
 */
void Commit(octetStream& comm, octetStream& open, const octetStream& message, int send_player);

bool Open(octetStream& message, const octetStream& comm, const octetStream& open, int send_player);

#endif


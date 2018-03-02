// (C) 2018 University of Bristol. See License.txt

#ifndef _NETWORK_STS
#define _NETWORK_STS

/* The Station to Station protocol
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <sodium.h>

using namespace std;

typedef enum
    { UNKNOWN  // Have not started the interaction or have cleared the memory
    , SENT1    // Sent initial message
    , SENT2    // Received 1, sent 2
    , FINISHED // Done (received msg 2 & sent 3 or received msg 3)
    , UNDEFINED // For arrays/vectors/etc of STS classes that are initialized later.
} phase_t;

struct msg1_st {
    unsigned char bytes[crypto_box_PUBLICKEYBYTES];
};
typedef struct msg1_st sts_msg1_t;
struct msg2_st {
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES];
    unsigned char sig[crypto_sign_BYTES];
};
typedef struct msg2_st sts_msg2_t;
struct msg3_st {
    unsigned char bytes[crypto_sign_BYTES];
};
typedef struct msg3_st sts_msg3_t;

class STS
{
    phase_t phase;
    unsigned char their_public_sign_key[crypto_sign_PUBLICKEYBYTES];
    unsigned char my_public_sign_key[crypto_sign_PUBLICKEYBYTES];
    unsigned char my_private_sign_key[crypto_sign_SECRETKEYBYTES];
    unsigned char ephemeral_private_key[crypto_box_SECRETKEYBYTES];
    unsigned char ephemeral_public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char their_ephemeral_public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char raw_secret[crypto_hash_sha512_BYTES];
    uint64_t kdf_counter;
  public:
    STS();
    STS( const unsigned char theirPub[crypto_sign_PUBLICKEYBYTES]
       , const unsigned char myPub[crypto_sign_PUBLICKEYBYTES]
       , const unsigned char myPriv[crypto_sign_SECRETKEYBYTES]);
    ~STS();

    void init( const unsigned char theirPub[crypto_sign_PUBLICKEYBYTES]
             , const unsigned char myPub[crypto_sign_PUBLICKEYBYTES]
             , const unsigned char myPriv[crypto_sign_SECRETKEYBYTES]);

    sts_msg1_t send_msg1();
    sts_msg3_t recv_msg2(sts_msg2_t msg2);

    sts_msg2_t recv_msg1(sts_msg1_t msg1);
    void recv_msg3(sts_msg3_t msg3);

    vector<unsigned char> derive_secret(size_t);
  private:
    vector<unsigned char> unsafe_derive_secret(size_t);
    void kdf_block(unsigned char *block);
};

#endif /* _NETWORK_STS */

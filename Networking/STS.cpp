// (C) 2018 University of Bristol. See License.txt

#include "Networking/STS.h"
#include <sodium.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iomanip>
#include <fcntl.h>

void STS::kdf_block(unsigned char *block)
{
    crypto_hash_sha512_state state;
    crypto_hash_sha512_init(&state);
    unsigned char ctrbytes[sizeof kdf_counter];
    kdf_counter++;

    // Little endian serialization
    for(size_t i=0; i<sizeof(kdf_counter); i++) {
        ctrbytes[i] = (unsigned char)((kdf_counter >> i*8) & 0xFF);
    }
    crypto_hash_sha512_update(&state,ctrbytes,sizeof ctrbytes);
    crypto_hash_sha512_update(&state,raw_secret,crypto_hash_sha512_BYTES);
    crypto_hash_sha512_final(&state, block);
}

vector<unsigned char> STS::unsafe_derive_secret(size_t sz)
{
    // KDF ~ H(cnt || raw_secret)
    vector<unsigned char> resultSecret(sz + crypto_hash_sha512_BYTES - (sz % crypto_hash_sha512_BYTES));
    size_t total=0;
    while(total < sz) {
        unsigned char *block = &resultSecret[total];
        kdf_block(block);
        total += crypto_hash_sha512_BYTES;
    }
    return resultSecret;
}

STS::STS()
{
    phase = UNDEFINED;
}

void STS::init( const unsigned char theirPub[crypto_sign_PUBLICKEYBYTES]
              , const unsigned char myPub[crypto_sign_PUBLICKEYBYTES]
              , const unsigned char myPriv[crypto_sign_SECRETKEYBYTES])
{
    phase = UNKNOWN;
    memcpy(their_public_sign_key, theirPub, crypto_sign_PUBLICKEYBYTES);
    memcpy(my_public_sign_key, myPub, crypto_sign_PUBLICKEYBYTES);
    memcpy(my_private_sign_key, myPriv, crypto_sign_SECRETKEYBYTES);
    memset(their_ephemeral_public_key, 0, crypto_box_PUBLICKEYBYTES);
    memset(ephemeral_public_key, 0, crypto_box_PUBLICKEYBYTES);
    memset(ephemeral_private_key, 0, crypto_box_SECRETKEYBYTES);
    kdf_counter = 0;
}

STS::STS( const unsigned char theirPub[crypto_sign_PUBLICKEYBYTES]
        , const unsigned char myPub[crypto_sign_PUBLICKEYBYTES]
        , const unsigned char myPriv[crypto_sign_SECRETKEYBYTES])
{
    phase = UNKNOWN;
    memcpy(their_public_sign_key, theirPub, crypto_sign_PUBLICKEYBYTES);
    memcpy(my_public_sign_key, myPub, crypto_sign_PUBLICKEYBYTES);
    memcpy(my_private_sign_key, myPriv, crypto_sign_SECRETKEYBYTES);
    memset(their_ephemeral_public_key, 0, crypto_box_PUBLICKEYBYTES);
    memset(ephemeral_public_key, 0, crypto_box_PUBLICKEYBYTES);
    memset(ephemeral_private_key, 0, crypto_box_SECRETKEYBYTES);
    kdf_counter = 0;
}

STS::~STS()
{
    memset(their_public_sign_key, 0, crypto_sign_PUBLICKEYBYTES);
    memset(my_private_sign_key, 0, crypto_sign_SECRETKEYBYTES);
    memset(ephemeral_private_key, 0, crypto_box_SECRETKEYBYTES);
    memset(ephemeral_public_key, 0, crypto_box_PUBLICKEYBYTES);
    memset(their_ephemeral_public_key, 0, crypto_box_PUBLICKEYBYTES);
    memset(raw_secret, 0, crypto_hash_sha512_BYTES);
    kdf_counter = 0;
    phase = UNKNOWN;
}

sts_msg1_t STS::send_msg1()
{
    sts_msg1_t m;
    if(UNKNOWN != phase) {
        throw "STS BAD PHASE";
    }

    crypto_box_keypair(ephemeral_public_key, ephemeral_private_key);
    memcpy(m.bytes,ephemeral_public_key,crypto_box_PUBLICKEYBYTES);
    phase = SENT1;
    return m;
}

// If the incoming signature is valid, compute:
//       shared secret = H(DH(pubB,privA) || pubA || pubB)
//       msg = Sign_{privED-A} (pubA || pubB )
//
sts_msg3_t STS::recv_msg2(sts_msg2_t msg2)
{
    unsigned char *theirPublicKey = msg2.pubkey;
    unsigned char *theirSig       = msg2.sig;
    unsigned char theirSigDec[crypto_sign_BYTES];
    unsigned char scalar_result[crypto_scalarmult_SCALARBYTES];
    const unsigned char zeroNonce[crypto_stream_NONCEBYTES] = {0};
    int ret;
    crypto_hash_sha512_state state;
    sts_msg3_t msg;

    if(SENT1 != phase) {
        throw "STS BAD PHASE";
    }
    ret = crypto_scalarmult(scalar_result, ephemeral_private_key, theirPublicKey);
    if(0 != ret) {
        throw "crypto_scalarmult failed";
    }

    crypto_hash_sha512_init(&state);
    crypto_hash_sha512_update(&state,scalar_result,crypto_scalarmult_SCALARBYTES);
    crypto_hash_sha512_update(&state,ephemeral_public_key,crypto_box_PUBLICKEYBYTES);
    crypto_hash_sha512_update(&state,theirPublicKey,crypto_box_PUBLICKEYBYTES);
    crypto_hash_sha512_final(&state,raw_secret);

    vector<unsigned char> keKey = unsafe_derive_secret(crypto_stream_KEYBYTES);
    vector<unsigned char> expectedMessage;
    expectedMessage.insert(expectedMessage.end(), theirPublicKey , theirPublicKey + crypto_box_PUBLICKEYBYTES);
    expectedMessage.insert(expectedMessage.end(), ephemeral_public_key,    ephemeral_public_key + crypto_box_PUBLICKEYBYTES);

    crypto_stream_xor(theirSigDec, theirSig, crypto_sign_BYTES, zeroNonce, &keKey[0]);

    int badSig = crypto_sign_verify_detached(theirSigDec, &expectedMessage[0], expectedMessage.size(), their_public_sign_key);

    if(badSig) {
        throw "Bad signature received in message 2.";
    } else {
        unsigned char *mySigEnc = msg.bytes;
        unsigned char mySig[crypto_sign_BYTES];
        vector<unsigned char> signMessage;
        signMessage.insert(signMessage.end(), ephemeral_public_key, ephemeral_public_key + crypto_box_PUBLICKEYBYTES);
        signMessage.insert(signMessage.end(), theirPublicKey , theirPublicKey + crypto_box_PUBLICKEYBYTES);
        if(0 != crypto_sign_detached(mySig, NULL, &signMessage[0], signMessage.size(), my_private_sign_key)) {
            throw "Signing failed.";
        }
        vector<unsigned char> keKey2 = unsafe_derive_secret(crypto_stream_KEYBYTES);
        crypto_stream_xor(mySigEnc, mySig, crypto_sign_BYTES, zeroNonce, &keKey2[0]);

        phase = FINISHED;
        return msg;
    }
}

sts_msg2_t STS::recv_msg1(sts_msg1_t msg1)
{
    unsigned char *theirPublicKey = msg1.bytes;
    unsigned char scalar_result[crypto_scalarmult_SCALARBYTES];
    crypto_hash_sha512_state state;
    sts_msg2_t m;
    int ret;

    if(UNKNOWN != phase) {
        throw "recv_msg1 called on non-unknown phase";
    }

    memcpy(their_ephemeral_public_key, theirPublicKey, crypto_box_PUBLICKEYBYTES);

    crypto_box_keypair(ephemeral_public_key, ephemeral_private_key);
    memcpy(m.pubkey,ephemeral_public_key,crypto_box_PUBLICKEYBYTES);
    ret = crypto_scalarmult(scalar_result, ephemeral_private_key, theirPublicKey);
    if(0 != ret) {
        throw "crypto_scalarmult failed when processing message 1";
    }

    crypto_hash_sha512_init(&state);
    crypto_hash_sha512_update(&state,scalar_result,crypto_scalarmult_SCALARBYTES);
    crypto_hash_sha512_update(&state,theirPublicKey,crypto_box_PUBLICKEYBYTES);
    crypto_hash_sha512_update(&state,ephemeral_public_key,crypto_box_PUBLICKEYBYTES);
    crypto_hash_sha512_final(&state,raw_secret);

    vector<unsigned char> livenessProof;
    livenessProof.insert(livenessProof.end(), ephemeral_public_key,    ephemeral_public_key + crypto_box_PUBLICKEYBYTES);
    livenessProof.insert(livenessProof.end(), theirPublicKey , theirPublicKey + crypto_box_PUBLICKEYBYTES);
    unsigned char mySig[crypto_sign_BYTES];
    unsigned char *mySigEnc = m.sig;
    vector<unsigned char> keKey = unsafe_derive_secret(crypto_stream_KEYBYTES);

    unsigned char zeroNonce[crypto_stream_NONCEBYTES] = {0};
    if(0 != crypto_sign_detached(mySig, NULL, &livenessProof[0], livenessProof.size(), my_private_sign_key)) {
        throw "Signing failed.";
    }
    crypto_stream_xor(mySigEnc, mySig, crypto_sign_BYTES, zeroNonce, &keKey[0]);

    phase = SENT2;
    return m;
}

void STS::recv_msg3(sts_msg3_t msg3)
{
    unsigned char *theirSig=msg3.bytes;
    unsigned char theirSigDec[crypto_sign_BYTES];
    vector<unsigned char> expectedMessage;
    if(SENT2 != phase) {
        throw "recv_msg3 called out of order";
    }

    expectedMessage.insert(expectedMessage.end(), their_ephemeral_public_key , their_ephemeral_public_key + crypto_box_PUBLICKEYBYTES);
    expectedMessage.insert(expectedMessage.end(), ephemeral_public_key,    ephemeral_public_key + crypto_box_PUBLICKEYBYTES);
    unsigned char zeroNonce[crypto_stream_NONCEBYTES] = {0};
    vector<unsigned char> keKey2 = unsafe_derive_secret(crypto_stream_KEYBYTES);

    crypto_stream_xor(theirSigDec, theirSig, crypto_sign_BYTES, zeroNonce, &keKey2[0]);
    int badSig = crypto_sign_verify_detached(theirSigDec, &expectedMessage[0], expectedMessage.size(), their_public_sign_key);

    if(badSig) {
        throw "Bad signature received in message 3.";
    } else {
        phase = FINISHED;
    }
}

vector<unsigned char> STS::derive_secret(size_t sz)
{
    if(phase != FINISHED) {
        throw "Can not derive secrets till the key exchange has completed.";
    }
    return unsafe_derive_secret(sz);
}

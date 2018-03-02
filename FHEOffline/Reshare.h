// (C) 2018 University of Bristol. See License.txt

#ifndef _Reshare
#define _Reshare

/* The procedure for the Reshare protocol 
 *   Input is a ciphertext cm and a flag NewCiphertext
 *   Output is a Ring_Element and possibly a ciphertext cc
 */

#include "FHE/Ciphertext.h"
#include "Networking/Player.h"
#include "FHEOffline/EncCommit.h"

template <class FD> class DistDecrypt;

template<class T,class FD,class S>
void Reshare(Plaintext<T,FD,S>& m,Ciphertext& cc,
             const Ciphertext& cm,bool NewCiphertext,
             const Player& P,EncCommitBase<T,FD,S>& EC,
             const FHE_PK& pk,const FHE_SK& share);

template<class T,class FD,class S>
void Reshare(Plaintext<T,FD,S>& m,Ciphertext& cc,
             const Ciphertext& cm,bool NewCiphertext,
             const Player& P,EncCommitBase<T,FD,S>& EC,
             const FHE_PK& pk,DistDecrypt<FD>& dd);

#endif


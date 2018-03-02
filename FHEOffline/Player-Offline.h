// (C) 2018 University of Bristol. See License.txt

/*
 * Player-Offline.h
 *
 */

#ifndef FHEOFFLINE_PLAYER_OFFLINE_H_
#define FHEOFFLINE_PLAYER_OFFLINE_H_

class thread_info
{
  public:

  int thread_num;
  int covert;
  Names*  Nms;
  FHE_PK* pk_p;
  FHE_PK* pk_2;
  FHE_SK* sk_p;
  FHE_SK* sk_2;
  Ciphertext *calphap;
  Ciphertext *calpha2;
  gfp *alphapi;
  gf2n_short *alpha2i;

  FFT_Data *FTD;
  P2Data *P2D;

  int nm2,nmp,nb2,nbp,ni2,nip,ns2,nsp,nvp;
  bool skip_2() { return nm2 + ni2 + nb2 + ns2 == 0; }
};

#endif /* FHEOFFLINE_PLAYER_OFFLINE_H_ */

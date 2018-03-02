// (C) 2018 University of Bristol. See License.txt

/*
 * DataSetup.h
 *
 */

#ifndef FHEOFFLINE_DATASETUP_H_
#define FHEOFFLINE_DATASETUP_H_

#include "Networking/Player.h"
#include "FHE/FHE_Keys.h"
#include "FHEOffline/FullSetup.h"
#include "Math/Setup.h"

class DataSetup;

template <class FD>
class PartSetup
{
public:
  typedef typename FD::T T;

  FHE_Params params;
  FD FieldD;
  FHE_PK pk;
  FHE_SK sk;
  Ciphertext calpha;
  typename FD::T alphai;

  PartSetup();
  void generate_setup(int n_parties, int plaintext_length, int sec, int slack,
      bool round_up);
  void output_setup(ostream& s) const;

  void fake(vector<FHE_SK>& sks, vector<T>& alphais, int nplayers, bool distributed = true);
  void fake(vector<PartSetup<FD> >& setups, int nplayers, bool distributed = true);
  void insecure_debug_keys(vector<PartSetup<FD> >& setups, int nplayers, bool simple_pk);

  void pack(octetStream& os);
  void unpack(octetStream& os);

  void init_field();

  void check(int sec) const;
  bool operator!=(const PartSetup<FD>& other);
};

class DataSetup
{
public:
  PartSetup<FFT_Data> setup_p;
  PartSetup<P2Data> setup_2;

  FHE_Params &params_p,&params_2;
  FFT_Data& FTD;
  P2Data& P2D;

  FHE_PK& pk_p;
  FHE_SK& sk_p;

  FHE_PK& pk_2;
  FHE_SK& sk_2;

  Ciphertext& calphap;
  Ciphertext& calpha2;

  gfp& alphapi;
  gf2n_short& alpha2i;

  DataSetup();
  DataSetup(Names & N, bool skip_2 = false);
  DataSetup(const DataSetup& other) : DataSetup() { *this = other; }
  DataSetup& operator=(const DataSetup& other);

  void write_setup(string dir, bool skip_2);
  void write_setup(bool skip_2);
  void write_setup(const Names& N, bool skip_2);
  string get_prep_dir(int n_parties) const;
  void read_setup(bool skip_2, string dir = PREP_DIR);
  void read(Names& N, bool skip_2 = false, string dir = PREP_DIR);
  void output(int my_number, int nn, bool specific_dir = false);
  template <class FD>
  PartSetup<FD>& part();
};

template<> inline PartSetup<FFT_Data>& DataSetup::part<FFT_Data>() { return setup_p; }
template<> inline PartSetup<P2Data>& DataSetup::part<P2Data>() { return setup_2; }

#endif /* FHEOFFLINE_DATASETUP_H_ */

// (C) 2018 University of Bristol. See License.txt

/*
 * DataSetup.cpp
 *
 */

#include <FHEOffline/DataSetup.h>
#include "FHEOffline/DistKeyGen.h"
#include "Auth/fake-stuff.h"
#include "FHE/NTL-Subs.h"
#include "Tools/benchmarking.h"

#include <iostream>
using namespace std;

template <class FD>
PartSetup<FD>::PartSetup() :
    pk(params, 0), sk(params, 0), calpha(params)
{
}

template <class FD>
void PartSetup<FD>::output_setup(ostream& s) const
{
  s << params.FFTD()[0].get_R() << endl;
  s << FieldD << endl;
  for (size_t i = 0; i < params.FFTD().size(); i++)
    {
      if (i != 0)
        s << " ";
      s << params.FFTD()[i].get_prime();
    }
  s << endl;
}

DataSetup::DataSetup() :
    params_p(setup_p.params), params_2(setup_2.params),
    FTD(setup_p.FieldD), P2D(setup_2.FieldD),
    pk_p(setup_p.pk), sk_p(setup_p.sk), pk_2(setup_2.pk), sk_2(setup_2.sk),
    calphap(setup_p.calpha), calpha2(setup_2.calpha),
    alphapi(setup_p.alphai), alpha2i(setup_2.alphai)
{
}

DataSetup::DataSetup(Names& N, bool skip_2) : DataSetup()
{
  read(N, skip_2);
}

DataSetup& DataSetup::operator=(const DataSetup& other)
{
  setup_p = other.setup_p;
  setup_2 = other.setup_2;
  return *this;
}

template <class FD>
void PartSetup<FD>::generate_setup(int n_parties, int plaintext_length, int sec,
    int slack, bool round_up)
{
  sec = max(sec, 40);
  ::generate_setup(n_parties, plaintext_length, sec, params, FieldD,
      slack, round_up);
  params.set_sec(sec);
  pk = FHE_PK(params, FieldD.get_prime());
  sk = FHE_SK(params, FieldD.get_prime());
  calpha = Ciphertext(params);
}

void DataSetup::write_setup(string dir, bool skip_2)
{
  ofstream outf;
  write_online_setup(outf, dir, FTD.get_prime(), gf2n::degree());
  setup_p.output_setup(outf);
  if (not skip_2)
    setup_2.output_setup(outf);
}

void DataSetup::write_setup(bool skip_2)
{
  write_setup(PREP_DIR, skip_2);
}

string DataSetup::get_prep_dir(int n_parties) const
{
  return ::get_prep_dir(n_parties, FTD.get_prime().numBits(),
      gf2n::degree());
}

void DataSetup::write_setup(const Names& N, bool skip_2)
{
  write_setup(get_prep_dir(N.num_players()), skip_2);
}

void DataSetup::read_setup(bool skip_2, string dir)
{
  cout << "Entering set up params" << endl;
  get_setup(params_p,FTD,params_2,P2D,dir,skip_2);
  // hack to make reading below work
  if (skip_2)
      params_2.set(FTD.get_R(), {params_p.FFTD()[0].get_prime(),
		  params_p.FFTD()[1].get_prime()});

  cout << "Left set up params" << endl;

  pk_p = FHE_PK(params_p,FTD.get_prime());
  sk_p = FHE_SK(params_p,FTD.get_prime());

  pk_2 = FHE_PK(params_2,P2D.get_prime());
  sk_2 = FHE_SK(params_2,P2D.get_prime());

  calphap = Ciphertext(params_p);
  calpha2 = Ciphertext(params_2);
}

void DataSetup::read(Names& N, bool skip_2, string dir)
{
  read_setup(skip_2, dir);

  int nn;
  string filename = dir + "/Player-FHE-Keys-P" + to_string(N.my_num());
  ifstream inpf(filename.c_str());
  if (inpf.fail()) { throw file_error(filename); }

  inpf >> nn;
  if (nn!=N.num_players())
    { cout << "KeyGen was last run with " << nn << " players." << endl;
      cout << "  - You are running Offline with " << N.num_players() << " players." << endl;
      exit(1);
    }

  /* Load in the public/private keys for this player */
  inpf >> pk_p >> sk_p;
  inpf >> pk_2 >> sk_2;

  /* Load in ciphertexts encrypting the MAC keys */
  inpf >> calphap;
  inpf >> calpha2;

  /* Loads in the players shares of the MAC keys */
  inpf >> alphapi;
  inpf >> alpha2i;

  inpf.close();

  cout << "Loaded the public keys etc" << endl;
}

template <class FD>
void PartSetup<FD>::fake(vector<FHE_SK>& sks, vector<T>& alphais,
        int nplayers, bool distributed)
{
  insecure("global key generation");
  if (distributed)
      cout << "Faking distributed key generation" << endl;
  else
      cout << "Faking key generation with extra noise" << endl;
  PRNG G;
  G.ReSeed();
  pk = FHE_PK(params, FieldD.get_prime());
  FHE_SK sk(params, FieldD.get_prime());
  calpha = Ciphertext(params);
  sks.resize(nplayers, pk);
  alphais.resize(nplayers);

  if (distributed)
      DistKeyGen::fake(pk, sks, FieldD.get_prime(), nplayers);
  else
  {
      Rq_Element sk = FHE_SK(pk).s();
      for (int i = 0; i < nplayers; i++)
      {
          Rq_Element ski = pk.sample_secret_key(G);
          sks[i].assign(ski);
          sk += ski;
      }
      pk.KeyGen(sk, G, nplayers);
  }

  for (int i = 0; i < nplayers; i++)
    {
      Plaintext_<FD> m(FieldD);
      m.randomize(G,Diagonal);
      Ciphertext calphai = pk.encrypt(m);
      calpha += calphai;
      alphais[i] = m.element(0);
    }
}

template <class FD>
void PartSetup<FD>::fake(vector<PartSetup<FD> >& setups, int nplayers,
        bool distributed)
{
    vector<FHE_SK> sks;
    vector<T> alphais;
    fake(sks, alphais, nplayers, distributed);
    setups.clear();
    setups.resize(nplayers, *this);
    for (int i = 0; i < nplayers; i++)
    {
        setups[i].sk = sks[i];
        setups[i].alphai = alphais[i];
    }
}

template <class FD>
void PartSetup<FD>::insecure_debug_keys(vector<PartSetup<FD> >& setups, int nplayers, bool simple_pk)
{
    cout << "generating INSECURE keys for debugging" << endl;
    setups.clear();
    Rq_Element zero(params, evaluation, evaluation),
            one(params, evaluation, evaluation);
    zero.assign_zero();
    one.assign_one();
    PRNG G;
    G.ReSeed();
    if (simple_pk)
        pk.assign(zero, zero, zero, zero - one);
    else
        pk.KeyGen(one, G, nplayers);
    setups.resize(nplayers, *this);
    setups[0].sk.assign(one);
    for (int i = 1; i < nplayers; i++)
        setups[i].sk.assign(zero);
}

void DataSetup::output(int my_number, int nn, bool specific_dir)
{
    // Write outputs to file
    string dir;
    if (specific_dir)
        dir = get_prep_dir(nn);
    else
        dir = PREP_DIR;
    string filename = dir + "Player-FHE-Keys-P" + to_string(my_number);
    cout << "Writing output to " << filename << endl;

    ofstream outf(filename.c_str());
    outf << nn << endl;
    outf << pk_p << "\n" << sk_p << endl;
    outf << pk_2 << "\n" << sk_2 << endl;
    // The encryption of the MAC keys
    outf << calphap << endl;
    outf << calpha2 << endl;
    // The players share of the MAC key is the constant term of mess
    alphapi.output(outf, true);
    outf << " ";
    alpha2i.output(outf, true);
    outf << endl;
    outf.close();

    write_mac_keys(dir, my_number, nn, alphapi, alpha2i);
}

template <class FD>
void PartSetup<FD>::pack(octetStream& os)
{
    params.pack(os);
    FieldD.pack(os);
    pk.pack(os);
    sk.pack(os);
    calpha.pack(os);
    alphai.pack(os);
}

template <class FD>
void PartSetup<FD>::unpack(octetStream& os)
{
    params.unpack(os);
    FieldD.unpack(os);
    pk.unpack(os);
    sk.unpack(os);
    calpha.unpack(os);
    init_field();
    alphai.unpack(os);
}

template <>
void PartSetup<FFT_Data>::init_field()
{
    gfp::init_field(FieldD.get_prime());
}

template <>
void PartSetup<P2Data>::init_field()
{
}

template <class FD>
void PartSetup<FD>::check(int sec) const
{
    sec = max(sec, 40);
    if (abs(sec - params.secp()) > 2)
        throw runtime_error("security parameters vary too much between protocol and distributed decryption");
    sk.check(params, pk, FieldD.get_prime());
}

template <class FD>
bool PartSetup<FD>::operator!=(const PartSetup<FD>& other)
{
    if (params != other.params or FieldD != other.FieldD or pk != other.pk
            or sk != other.sk or calpha != other.calpha
            or alphai != other.alphai)
        return true;
    else
        return false;
}

template class PartSetup<FFT_Data>;
template class PartSetup<P2Data>;

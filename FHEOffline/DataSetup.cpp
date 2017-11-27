// (C) 2017 University of Bristol. See License.txt

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
  ::generate_setup(n_parties, plaintext_length, sec, params, FieldD,
      slack, round_up);
  params.set_sec(sec);
  pk = FHE_PK(params, FieldD.get_prime());
  sk = FHE_SK(params, FieldD.get_prime());
  calpha = Ciphertext(params);
}

template <>
void DataSetup::generate_setup<gfp>(int n_parties, int plaintext_length, int sec,
    int slack, bool write_output)
{
  setup_p.generate_setup(n_parties, plaintext_length, sec, slack, true);
  if (write_output)
    write_setup(true);
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

template <>
void DataSetup::fake<gfp>(vector<FHE_SK>& sks, vector<gfp>& alphais,
        int nplayers, bool distributed)
{
  insecure("global key generation");
  if (distributed)
      cout << "Faking distributed key generation" << endl;
  else
      cout << "Faking key generation with extra noise" << endl;
  PRNG G;
  G.ReSeed();
  pk_p = FHE_PK(params_p, FTD.get_prime());
  FHE_SK sk(params_p, FTD.get_prime());
  calphap = Ciphertext(params_p);
  sks.resize(nplayers, pk_p);
  alphais.resize(nplayers);

  if (distributed)
      DistKeyGen::fake(pk_p, sks, FTD.get_prime(), nplayers);
  else
  {
      Rq_Element sk = FHE_SK(pk_p).s();
      for (int i = 0; i < nplayers; i++)
      {
          Rq_Element ski = pk_p.sample_secret_key(G);
          sks[i].assign(ski);
          sk += ski;
      }
      pk_p.KeyGen(sk, G, nplayers);
  }

  for (int i = 0; i < nplayers; i++)
    {
      Plaintext<gfp,FFT_Data,bigint> m(FTD);
      m.randomize(G,Diagonal);
      Ciphertext calphai = pk_p.encrypt(m);
      calphap += calphai;
      alphais[i] = m.element(0);
    }
}

template <>
void DataSetup::fake<gfp>(vector<DataSetup>& setups, int nplayers,
        bool distributed)
{
    vector<FHE_SK> sks;
    vector<gfp> alphais;
    fake<gfp>(sks, alphais, nplayers, distributed);
    setups.clear();
    setups.resize(nplayers, *this);
    for (int i = 0; i < nplayers; i++)
    {
        setups[i].sk_p = sks[i];
        setups[i].alphapi = alphais[i];
    }
}

void DataSetup::insecure_debug_keys(vector<DataSetup>& setups, int nplayers, bool simple_pk)
{
    cout << "generating INSECURE keys for debugging" << endl;
    setups.clear();
    Rq_Element zero(params_p, evaluation, evaluation),
            one(params_p, evaluation, evaluation);
    zero.assign_zero();
    one.assign_one();
    PRNG G;
    G.ReSeed();
    if (simple_pk)
        pk_p.assign(zero, zero, zero, zero - one);
    else
        pk_p.KeyGen(one, G, nplayers);
    setups.resize(nplayers, *this);
    setups[0].sk_p.assign(one);
    for (int i = 1; i < nplayers; i++)
        setups[i].sk_p.assign(zero);
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

template <>
void DataSetup::pack<gfp>(octetStream& os)
{
    params_p.pack(os);
    FTD.pack(os);
    pk_p.pack(os);
    sk_p.pack(os);
    calphap.pack(os);
    alphapi.pack(os);
}

template <>
void DataSetup::unpack<gfp>(octetStream& os)
{
    params_p.unpack(os);
    FTD.unpack(os);
    pk_p.unpack(os);
    sk_p.unpack(os);
    calphap.unpack(os);
    alphapi.unpack(os);
    gfp::init_field(FTD.get_prime());
}

void DataSetup::check(int sec) const
{
    if (sec < params_p.secp())
        throw runtime_error("security parameter for distributed decryption too big");
    sk_p.check(params_p, pk_p, FTD.get_prime());
}

bool DataSetup::operator!=(const DataSetup& other)
{
    if (params_p != other.params_p or FTD != other.FTD or pk_p != other.pk_p
            or sk_p != other.sk_p or calphap != other.calphap
            or alphapi != other.alphapi)
        return true;
    else
        return false;
}

template class PartSetup<FFT_Data>;
template class PartSetup<P2Data>;

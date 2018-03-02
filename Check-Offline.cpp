// (C) 2018 University of Bristol. See License.txt

/*
 * Check-Offline.cpp
 *
 */

#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Share.h"
#include "Auth/fake-stuff.h"
#include "Tools/ezOptionParser.h"
#include "Exceptions/Exceptions.h"

#include "Math/Setup.h"
#include "Processor/Data_Files.h"

#include <sstream>
#include <fstream>
#include <vector>
using namespace std;

string PREP_DATA_PREFIX;

template<class T>
void check_mult_triples(const T& key,int N,vector<Data_Files*>& dataF,DataFieldType field_type)
{
  T a,b,c,mac,res;
  vector<Share<T> > Sa(N),Sb(N),Sc(N);
  int n = 0;

  try {
      while (!dataF[0]->eof<T>(DATA_TRIPLE))
        {
          for (int i = 0; i < N; i++)
            dataF[i]->get_three(field_type, DATA_TRIPLE, Sa[i], Sb[i], Sc[i]);
          check_share(Sa, a, mac, N, key);
          check_share(Sb, b, mac, N, key);
          check_share(Sc, c, mac, N, key);

          res.mul(a, b);
          if (!res.equal(c))
            {
              cout << n << ": " << c << " != " << a << " * " << b << endl;
              throw bad_value();
            }
          n++;
        }

      cout << n << " triples of type " << T::type_string() << endl;
  }
  catch (exception& e)
  {
      cout << "Error after " << n << " triples of type " << T::type_string() << endl;
  }
}

template <class T>
void check_square(const T& a, const T& b, int n)
{
  T res;
  res.mul(a, a);
  if (!res.equal(b))
    {
      cout << n << ": " << b << " != " << a << "^2" << endl;
      throw bad_value();
    }
}

template <class T>
void check_inverse(const T& a, const T& b, int n)
{
  T res;
  res.mul(a, b);
  if (!res.is_one())
    {
      cout << n << ": " << b << " != " << a << "^-1" << endl;
      throw bad_value();
    }
}

template<class T>
void check_tuple(const T& a, const T& b, int n, Dtype type)
{
  if (type == DATA_SQUARE)
    check_square(a, b, n);
  else if (type == DATA_INVERSE)
    check_inverse(a, b, n);
  else
    throw runtime_error("type not supported");
}

template<class T>
void check_tuples(const T& key,int N,vector<Data_Files*>& dataF, Dtype type)
{
  T a,b,c,mac,res;
  vector<Share<T> > Sa(N),Sb(N),Sc(N);
  int n = 0;
  DataFieldType field_type = T::field_type();

  try {
      while (!dataF[0]->eof<T>(type))
        {
          for (int i = 0; i < N; i++)
            dataF[i]->get_two(field_type, type, Sa[i], Sb[i]);
          check_share(Sa, a, mac, N, key);
          check_share(Sb, b, mac, N, key);
          check_tuple(a, b, n, type);
          n++;
        }

        cout << n << " " << Data_Files::dtype_names[type] << " of type "
                << T::type_string() << endl;
  }
  catch (exception& e)
  {
      cout << "Error after " << n << " " << Data_Files::dtype_names[type] <<
              " of type " << T::type_string() << endl;
  }
}

template<class T>
void check_bits(const T& key,int N,vector<Data_Files*>& dataF,DataFieldType field_type)
{
  T a,b,c,mac,res;
  vector<Share<T> > Sa(N),Sb(N),Sc(N);
  int n = 0;

  try {
      while (!dataF[0]->eof<T>(DATA_BIT))
      {
          for (int i = 0; i < N; i++)
              dataF[i]->get_one(field_type, DATA_BIT, Sa[i]);
          check_share(Sa, a, mac, N, key);

          if (!(a.is_zero() || a.is_one()))
          {
              cout << n << ": " << a << " neither 0 or 1" << endl;
              throw bad_value();
          }
          n++;
      }

      cout << n << " bits of type " << T::type_string() << endl;
  }
  catch (exception& e)
  {
      cout << "Error after " << n << " bits of type " << T::type_string() << endl;
  }
}

template<class T>
void check_inputs(const T& key,int N,vector<Data_Files*>& dataF)
{
  T a, mac, x;
  vector< Share<T> > Sa(N);

  for (int player = 0; player < N; player++)
    {
      int n = 0;
      try {
          while (!dataF[0]->input_eof<T>(player))
          {
              for (int i = 0; i < N; i++)
                  dataF[i]->get_input(Sa[i], x, player);
              check_share(Sa, a, mac, N, key);
              if (!a.equal(x))
                  throw bad_value();
              n++;
          }
          cout << n << " input masks for player " << player << " of type " << T::type_string() << endl;
      }
      catch (exception& e)
      {
          cout << "Error after " << n << " input masks of type "
              << T::type_string() << " for player " << player << endl;
      }
  }
}

int main(int argc, const char** argv)
{
  ez::ezOptionParser opt;
  gfp::init_field(gfp::pr(), false);

  opt.syntax = "./Check-Offline.x <nparties> [OPTIONS]\n";
  opt.example = "./Check-Offline.x 3 -lgp 64 -lg2 128\n";

  opt.add(
        "128", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Bit length of GF(p) field (default: 128)", // Help description.
        "-lgp", // Flag token.
        "--lgp" // Flag token.
  );
  opt.add(
        "40", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Bit length of GF(2^n) field (default: 40)", // Help description.
        "-lg2", // Flag token.
        "--lg2" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Read GF(p) triples in Montgomery representation (default: not set)", // Help description.
        "-m", // Flag token.
        "--usemont" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Directory containing the data (default: " PREP_DIR "<nparties>-<lgp>-<lg2>", // Help description.
        "-d", // Flag token.
        "--dir" // Flag token.
  );
  opt.parse(argc, argv);

  string usage;
  int lgp, lg2, nparties;
  bool use_montgomery = false;
  opt.get("--lgp")->getInt(lgp);
  opt.get("--lg2")->getInt(lg2);
  if (opt.isSet("--usemont"))
        use_montgomery = true;

  if (opt.firstArgs.size() == 2)
      nparties = atoi(opt.firstArgs[1]->c_str());
  else if (opt.lastArgs.size() == 1)
      nparties = atoi(opt.lastArgs[0]->c_str());
  else
  {
      cerr << "ERROR: invalid number of arguments\n";
      opt.getUsage(usage);
      cout << usage;
      return 1;
  }

  if (opt.isSet("--dir"))
    {
      opt.get("--dir")->getString(PREP_DATA_PREFIX);
      PREP_DATA_PREFIX += "/";
    }
  else
    PREP_DATA_PREFIX = get_prep_dir(nparties, lgp, lg2);

  read_setup(PREP_DATA_PREFIX);

  if (!use_montgomery)
  {
    // no montgomery
    gfp::init_field(gfp::pr(), false);
  }

  /* Find number players and MAC keys etc*/
  char filename[1024];
  gfp keyp,pp; keyp.assign_zero();
  gf2n key2,p2; key2.assign_zero();
  int N=1;
  ifstream inpf;
  for (int i= 0; i < nparties; i++)
  {
      sprintf(filename, (PREP_DATA_PREFIX + "Player-MAC-Keys-P%d").c_str(), i);
      inpf.open(filename);
      if (inpf.fail()) { throw file_error(filename); }
      inpf >> N;
      pp.input(inpf,true);
      p2.input(inpf,true);
      cout << " Key " << i << "\t p: " << pp << "\n\t 2: " << p2 << endl;
      keyp.add(pp);
      key2.add(p2);
      inpf.close();
  }
  cout << "--------------\n";
  cout << "Final Keys :\t p: " << keyp << "\n\t\t 2: " << key2 << endl;

  vector<Data_Files*> dataF(N);
  for (int i = 0; i < N; i++)
    dataF[i] = new Data_Files(i, N, PREP_DATA_PREFIX);
  check_mult_triples(key2, N, dataF, DATA_GF2N);
  check_mult_triples(keyp, N, dataF, DATA_MODP);
  check_inputs(key2, N, dataF);
  check_inputs(keyp, N, dataF);
  check_bits(key2, N, dataF, DATA_GF2N);
  check_bits(keyp, N, dataF, DATA_MODP);
  check_tuples(key2, N, dataF, DATA_SQUARE);
  check_tuples(keyp, N, dataF, DATA_SQUARE);
  check_tuples(key2, N, dataF, DATA_INVERSE);
  check_tuples(keyp, N, dataF, DATA_INVERSE);
  for (int i = 0; i < N; i++)
    delete dataF[i];
}

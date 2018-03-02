// (C) 2018 University of Bristol. See License.txt

#ifndef _Data_Files
#define _Data_Files

/* This class holds the Online data files all in one place
 * so the streams are easy to pass around and access
 */

#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Math/Share.h"
#include "Math/field_types.h"
#include "Processor/Buffer.h"
#include "Processor/InputTuple.h"
#include "Tools/Lock.h"
#include "Networking/Player.h"

#include <fstream>
#include <map>
using namespace std;

enum Dtype { DATA_TRIPLE, DATA_SQUARE, DATA_BIT, DATA_INVERSE, DATA_BITTRIPLE, DATA_BITGF2NTRIPLE, N_DTYPE };

class DataTag
{
  int t[4];

public:
  // assume that tag is three integers
  DataTag(const int* tag)
  {
    strncpy((char*)t, (char*)tag, 3 * sizeof(int));
    t[3] = 0;
  }
  string get_string() const
  {
    return string((char*)t);
  }
  bool operator<(const DataTag& other) const
  {
    for (int i = 0; i < 3; i++)
      if (t[i] != other.t[i])
        return t[i] < other.t[i];
    return false;
  }
};

struct DataPositions
{
  vector< vector<int> > files;
  vector< vector<int> > inputs;
  map<DataTag, int> extended[N_DATA_FIELD_TYPE];

  DataPositions(int num_players = 0) { set_num_players(num_players); }
  void set_num_players(int num_players);
  void increase(const DataPositions& delta);
  void print_cost() const;
};

class Processor;

class Data_Files
{
  static map<DataTag, int> tuple_lengths;
  static Lock tuple_lengths_lock;

  BufferHelper<Share, Share> buffers[N_DTYPE];
  BufferHelper<Share, Share>* input_buffers;
  BufferHelper<InputTuple, RefInputTuple> my_input_buffers;
  map<DataTag, BufferHelper<Share, Share> > extended;

  int my_num,num_players;

  DataPositions usage;

  public:

  const string prep_data_dir;

  static const char* dtype_names[N_DTYPE];
  static const char* field_names[N_DATA_FIELD_TYPE];
  static const char* long_field_names[N_DATA_FIELD_TYPE];
  static const bool implemented[N_DATA_FIELD_TYPE][N_DTYPE];
  static const int tuple_size[N_DTYPE];

  static int share_length(int field_type);
  static int tuple_length(int field_type, int dtype);

  Data_Files(int my_num,int n,const string& prep_data_dir);
  Data_Files(Names& N, const string& prep_data_dir) :
      Data_Files(N.my_num(), N.num_players(), prep_data_dir) {}
  ~Data_Files();

  DataPositions tellg();
  void seekg(DataPositions& pos);
  void skip(const DataPositions& pos);
  void prune();
  void purge();

  template<class T>
  bool eof(Dtype dtype);
  template<class T>
  bool input_eof(int player);

  void setup_extended(DataFieldType field_type, const DataTag& tag, int tuple_size = 0);
  template<class T>
  void get(Processor& proc, DataTag tag, const vector<int>& regs, int vector_size);

  DataPositions get_usage()
  {
    return usage;
  }

  template <class T>
  void get_three(DataFieldType field_type, Dtype dtype, Share<T>& a, Share<T>& b, Share<T>& c)
  {
    usage.files[field_type][dtype]++;
    buffers[dtype].input(a);
    buffers[dtype].input(b);
    buffers[dtype].input(c);
  }

  template <class T>
  void get_two(DataFieldType field_type, Dtype dtype, Share<T>& a, Share<T>& b)
  {
    usage.files[field_type][dtype]++;
    buffers[dtype].input(a);
    buffers[dtype].input(b);
  }

  template <class T>
  void get_one(DataFieldType field_type, Dtype dtype, Share<T>& a)
  {
    usage.files[field_type][dtype]++;
    buffers[dtype].input(a);
  }

  template <class T>
  void get_input(Share<T>& a,T& x,int i)
  {
    usage.inputs[i][T::field_type()]++;
    RefInputTuple<T> tuple(a, x);
    if (i==my_num)
      my_input_buffers.input(tuple);
    else
      input_buffers[i].input(a);
  }
};

template<class T> inline
bool Data_Files::eof(Dtype dtype)
  { return buffers[dtype].get_buffer(T::field_type()).eof; }

template<class T> inline
bool Data_Files::input_eof(int player)
{
  if (player == my_num)
    return my_input_buffers.get_buffer(T::field_type()).eof;
  else
    return input_buffers[player].get_buffer(T::field_type()).eof;
}

#endif

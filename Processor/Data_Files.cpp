// (C) 2018 University of Bristol. See License.txt


#include "Processor/Data_Files.h"
#include "Processor/Processor.h"

#include <iomanip>

const char* Data_Files::field_names[] = { "p", "2" };
const char* Data_Files::long_field_names[] = { "gfp", "gf2n" };
const bool Data_Files::implemented[N_DATA_FIELD_TYPE][N_DTYPE] = {
    { true, true, true, true, false, false },
    { true, true, true, true, true, true },
};
const int Data_Files::tuple_size[N_DTYPE] = { 3, 2, 1, 2, 3, 3 };

Lock Data_Files::tuple_lengths_lock;
map<DataTag, int> Data_Files::tuple_lengths;


void DataPositions::set_num_players(int num_players)
{
  files.resize(N_DATA_FIELD_TYPE, vector<int>(N_DTYPE));
  inputs.resize(num_players, vector<int>(N_DATA_FIELD_TYPE));
}

void DataPositions::increase(const DataPositions& delta)
{
  if (inputs.size() != delta.inputs.size())
    throw invalid_length();
  for (unsigned int field_type = 0; field_type < N_DATA_FIELD_TYPE; field_type++)
    {
      for (unsigned int dtype = 0; dtype < N_DTYPE; dtype++)
        files[field_type][dtype] += delta.files[field_type][dtype];
      for (unsigned int j = 0; j < inputs.size(); j++)
        inputs[j][field_type] += delta.inputs[j][field_type];

      map<DataTag, int>::const_iterator it;
      const map<DataTag, int>& delta_ext = delta.extended[field_type];
      for (it = delta_ext.begin(); it != delta_ext.end(); it++)
          extended[field_type][it->first] += it->second;
    }
}

void DataPositions::print_cost() const
{
  ifstream file("cost");
  double total_cost = 0;
  for (int i = 0; i < N_DATA_FIELD_TYPE; i++)
    {
      cerr << "  Type " << Data_Files::field_names[i] << endl;
      for (int j = 0; j < N_DTYPE; j++)
        {
          double cost_per_item = 0;
          file >> cost_per_item;
          if (cost_per_item < 0)
            break;
          long long items_used = files[i][j];
          double cost = items_used * cost_per_item;
          total_cost += cost;
          cerr.fill(' ');
          cerr << "    " << setw(10) << cost << " = " << setw(10) << items_used
              << " " << setw(14) << Data_Files::dtype_names[j] << " à " << setw(11)
              << cost_per_item << endl;
        }
        for (map<DataTag, int>::const_iterator it = extended[i].begin();
                it != extended[i].end(); it++)
        {
          cerr.fill(' ');
          cerr << setw(27) << it->second << " " << setw(14) << it->first.get_string() << endl;
        }
    }

  cerr << "Total cost: " << total_cost << endl;
}


int Data_Files::share_length(int field_type)
{
  switch (field_type)
  {
    case DATA_MODP:
      return 2 * gfp::t() * sizeof(mp_limb_t);
    case DATA_GF2N:
      return Share<gf2n>::size();
    default:
      throw invalid_params();
  }
}

int Data_Files::tuple_length(int field_type, int dtype)
{
  return tuple_size[dtype] * share_length(field_type);
}

Data_Files::Data_Files(int myn, int n, const string& prep_data_dir) :
    usage(n), prep_data_dir(prep_data_dir)
{
  cerr << "Setting up Data_Files in: " << prep_data_dir << endl;
  num_players=n;
  my_num=myn;
  char filename[1024];
  input_buffers = new BufferHelper<Share, Share>[num_players];

  for (int field_type = 0; field_type < N_DATA_FIELD_TYPE; field_type++)
    {
      for (int dtype = 0; dtype < N_DTYPE; dtype++)
        {
          if (implemented[field_type][dtype])
            {
              sprintf(filename,(prep_data_dir + "%s-%s-P%d").c_str(),dtype_names[dtype],
                  field_names[field_type],my_num);
                buffers[dtype].setup(DataFieldType(field_type), filename,
                        tuple_length(field_type, dtype), dtype_names[dtype]);
            }
        }

      for (int i=0; i<num_players; i++)
        {
          sprintf(filename,(prep_data_dir + "Inputs-%s-P%d-%d").c_str(),
              field_names[field_type],my_num,i);
          if (i == my_num)
                my_input_buffers.setup(DataFieldType(field_type), filename,
                        share_length(field_type) * 3 / 2);
          else
                input_buffers[i].setup(DataFieldType(field_type), filename,
                        share_length(field_type));
        }
    }

  cerr << "done\n";
}

Data_Files::~Data_Files()
{
  for (int i = 0; i < N_DTYPE; i++)
    buffers[i].close();
  for (int i = 0; i < num_players; i++)
    input_buffers[i].close();
  delete[] input_buffers;
  my_input_buffers.close();
  for (map<DataTag, BufferHelper<Share, Share> >::iterator it =
      extended.begin(); it != extended.end(); it++)
    it->second.close();
}

void Data_Files::seekg(DataPositions& pos)
{
  for (int field_type = 0; field_type < N_DATA_FIELD_TYPE; field_type++)
    {
      for (int dtype = 0; dtype < N_DTYPE; dtype++)
        if (implemented[field_type][dtype])
          buffers[dtype].get_buffer(DataFieldType(field_type)).seekg(pos.files[field_type][dtype]);
      for (int j = 0; j < num_players; j++)
        if (j == my_num)
          my_input_buffers.get_buffer(DataFieldType(field_type)).seekg(pos.inputs[j][field_type]);
        else
          input_buffers[j].get_buffer(DataFieldType(field_type)).seekg(pos.inputs[j][field_type]);
      for (map<DataTag, int>::const_iterator it = pos.extended[field_type].begin();
          it != pos.extended[field_type].end(); it++)
        {
          setup_extended(DataFieldType(field_type), it->first);
          extended[it->first].get_buffer(DataFieldType(field_type)).seekg(it->second);
        }
    }

  usage = pos;
}

void Data_Files::skip(const DataPositions& pos)
{
  DataPositions new_pos = usage;
  new_pos.increase(pos);
  seekg(new_pos);
}

void Data_Files::prune()
{
  for (auto& buffer : buffers)
    buffer.prune();
  my_input_buffers.prune();
  for (int j = 0; j < num_players; j++)
    input_buffers[j].prune();
  for (auto it : extended)
    it.second.prune();
}

void Data_Files::purge()
{
  for (auto& buffer : buffers)
    buffer.purge();
  my_input_buffers.purge();
  for (int j = 0; j < num_players; j++)
    input_buffers[j].purge();
  for (auto it : extended)
    it.second.purge();
}

void Data_Files::setup_extended(DataFieldType field_type, const DataTag& tag, int tuple_size)
{
  BufferBase& buffer = extended[tag].get_buffer(field_type);
  tuple_lengths_lock.lock();
  int tuple_length = tuple_lengths[tag];
  int my_tuple_length = tuple_size * share_length(field_type);
  if (tuple_length > 0)
    {
      if (tuple_size > 0 && my_tuple_length != tuple_length)
        {
          stringstream ss;
          ss << "Inconsistent size of " << field_names[field_type] << " "
              << tag.get_string() << ": " << my_tuple_length << " vs "
              << tuple_length;
          throw Processor_Error(ss.str());
        }
    }
  else
    tuple_lengths[tag] = my_tuple_length;
  tuple_lengths_lock.unlock();

  if (!buffer.is_up())
    {
      stringstream ss;
      ss << prep_data_dir << tag.get_string() << "-" << field_names[field_type] << "-P" << my_num;
      extended[tag].setup(field_type, ss.str(), tuple_length);
    }
}

template<class T>
void Data_Files::get(Processor& proc, DataTag tag, const vector<int>& regs, int vector_size)
{
  usage.extended[T::field_type()][tag] += vector_size;
  setup_extended(T::field_type(), tag, regs.size());
  for (int j = 0; j < vector_size; j++)
    for (unsigned int i = 0; i < regs.size(); i++)
      extended[tag].input(proc.get_S_ref<T>(regs[i] + j));
}

template void Data_Files::get<gfp>(Processor& proc, DataTag tag, const vector<int>& regs, int vector_size);
template void Data_Files::get<gf2n>(Processor& proc, DataTag tag, const vector<int>& regs, int vector_size);

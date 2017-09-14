// (C) 2017 University of Bristol. See License.txt

/*
 * Buffer.cpp
 *
 */

#include "Buffer.h"
#include "Processor/InputTuple.h"

bool BufferBase::rewind = false;


void BufferBase::setup(ifstream* f, int length, const char* type)
{
    file = f;
    tuple_length = length;
    data_type = type;
}

void BufferBase::seekg(int pos)
{
    file->seekg(pos * tuple_length);
    if (file->eof() || file->fail())
    {
        file->clear();
        file->seekg(0);
        if (!rewind)
            cerr << "REWINDING - ONLY FOR BENCHMARKING" << endl;
        rewind = true;
    }
    next = BUFFER_SIZE;
}

template<class T, class U>
Buffer<T, U>::~Buffer()
{
    if (timer.elapsed() && data_type)
        cerr << T::type_string() << " " << data_type << " reading: "
                << timer.elapsed() << endl;
}

template<class T, class U>
void Buffer<T, U>::fill_buffer()
{
  if (T::size() == sizeof(T))
    {
      // read directly
      read((char*)buffer);
    }
  else
    {
      char read_buffer[sizeof(buffer)];
      read(read_buffer);
      //memset(buffer, 0, sizeof(buffer));
      for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i].assign(&read_buffer[i*T::size()]);
    }
}

template<class T, class U>
void Buffer<T, U>::read(char* read_buffer)
{
    int size_in_bytes = T::size() * BUFFER_SIZE;
    int n_read = 0;
    timer.start();
    do
    {
        file->read(read_buffer + n_read, size_in_bytes - n_read);
        n_read += file->gcount();
        if (file->eof())
        {
            file->clear(); // unset EOF flag
            file->seekg(0);
            if (!rewind)
                cerr << "REWINDING - ONLY FOR BENCHMARKING" << endl;
            rewind = true;
            eof = true;
        }
        if (file->fail())
          {
            stringstream ss;
            ss << "IO problem when buffering " << T::type_string();
            if (data_type)
              ss << " " << data_type;
            throw file_error(ss.str());
          }
    }
    while (n_read < size_in_bytes);
    timer.stop();
}

template <class T, class U>
void Buffer<T,U>::input(U& a)
{
    if (next == BUFFER_SIZE)
    {
        fill_buffer();
        next = 0;
    }

    a = buffer[next];
    next++;
}

template < template<class T> class U, template<class T> class V >
BufferBase& BufferHelper<U,V>::get_buffer(DataFieldType field_type)
{
    if (field_type == DATA_MODP)
        return bufferp;
    else if (field_type == DATA_GF2N)
        return buffer2;
    else
        throw not_implemented();
}

template < template<class T> class U, template<class T> class V >
void BufferHelper<U,V>::setup(DataFieldType field_type, string filename, int tuple_length, const char* data_type)
{
    files[field_type] = new ifstream(filename.c_str(), ios::in | ios::binary);
    if (files[field_type]->fail())
      throw file_error(filename);
    get_buffer(field_type).setup(files[field_type], tuple_length, data_type);
}

template<template<class T> class U, template<class T> class V>
void BufferHelper<U,V>::close()
{
    for (int i = 0; i < N_DATA_FIELD_TYPE; i++)
        if (files[i])
        {
            files[i]->close();
            delete files[i];
        }
}

template class Buffer< Share<gfp>, Share<gfp> >;
template class Buffer< Share<gf2n>, Share<gf2n> >;
template class Buffer< InputTuple<gfp>, RefInputTuple<gfp> >;
template class Buffer< InputTuple<gf2n>, RefInputTuple<gf2n> >;
template class Buffer< gfp, gfp >;
template class Buffer< gf2n, gf2n >;

template class BufferHelper<Share, Share>;
template class BufferHelper<InputTuple, RefInputTuple>;

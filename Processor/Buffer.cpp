// (C) 2018 University of Bristol. See License.txt

/*
 * Buffer.cpp
 *
 */

#include "Buffer.h"
#include "Processor/InputTuple.h"
#include "Processor/Data_Files.h"

bool BufferBase::rewind = false;


void BufferBase::setup(ifstream* f, int length, string filename,
        const char* type, const char* field)
{
    file = f;
    tuple_length = length;
    data_type = type;
    field_type = field;
    this->filename = filename;
}

void BufferBase::seekg(int pos)
{
    file->seekg(pos * tuple_length);
    if (file->eof() || file->fail())
    {
        // let it go in case we don't need it anyway
        if (pos != 0)
            try_rewind();
    }
    next = BUFFER_SIZE;
}

void BufferBase::try_rewind()
{
#ifndef INSECURE
    string type;
    if (field_type and data_type)
        type = (string)" of " + field_type + " " + data_type;
    throw not_enough_to_buffer(type);
#endif
    file->clear(); // unset EOF flag
    file->seekg(0);
    if (file->peek() == ifstream::traits_type::eof())
        throw runtime_error("empty file: " + filename);
    if (!rewind)
        cerr << "REWINDING - ONLY FOR BENCHMARKING" << endl;
    rewind = true;
    eof = true;
}

void BufferBase::prune()
{
    if (file and file->tellg() != 0)
    {
        cerr << "Pruning " << filename << endl;
        string tmp_name = filename + ".new";
        ofstream tmp(tmp_name.c_str());
        tmp << file->rdbuf();
        tmp.close();
        file->close();
        rename(tmp_name.c_str(), filename.c_str());
        file->open(filename.c_str(), ios::in | ios::binary);
    }
}

void BufferBase::purge()
{
    if (file)
    {
        cerr << "Removing " << filename << endl;
        unlink(filename.c_str());
        file->close();
        file = 0;
    }
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
            try_rewind();
        }
        if (file->fail())
          {
            stringstream ss;
            ss << "IO problem when buffering " << T::type_string();
            if (data_type)
              ss << " " << data_type;
            ss << " from " << filename;
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
    get_buffer(field_type).setup(files[field_type], tuple_length, filename,
            data_type, Data_Files::long_field_names[field_type]);
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

template<template<class T> class U, template<class T> class V>
void BufferHelper<U,V>::prune()
{
    buffer2.prune();
    bufferp.prune();
}

template<template<class T> class U, template<class T> class V>
void BufferHelper<U,V>::purge()
{
    buffer2.purge();
    bufferp.purge();
}

template class Buffer< Share<gfp>, Share<gfp> >;
template class Buffer< Share<gf2n>, Share<gf2n> >;
template class Buffer< InputTuple<gfp>, RefInputTuple<gfp> >;
template class Buffer< InputTuple<gf2n>, RefInputTuple<gf2n> >;
template class Buffer< gfp, gfp >;
template class Buffer< gf2n, gf2n >;

template class BufferHelper<Share, Share>;
template class BufferHelper<InputTuple, RefInputTuple>;

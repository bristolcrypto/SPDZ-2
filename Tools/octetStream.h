// (C) 2016 University of Bristol. See License.txt

#ifndef _octetStream
#define _octetStream


/* This class creates a stream of data and adds stuff onto it.
 * This is used to pack and unpack stuff which is sent over the
 * network
 *
 * Unlike SPDZ-1.0 this class ONLY deals with native types
 * For our types we assume pack/unpack operations defined within
 * that class. This is to make sure this class is relatively independent
 * of the rest of the application; and so can be re-used.
 */


#include "Math/bigint.h"
#include "Networking/data.h"
#include "Networking/sockets.h"

#include <string.h>
#include <vector>
#include <stdio.h>
#include <iostream>
using namespace std;


class octetStream
{
  int len,mxlen,ptr;  // len is the "write head", ptr is the "read head"
  octet *data;

  public:

  void resize(int l);

  void assign(const octetStream& os);

  octetStream() : len(0), mxlen(0), ptr(0), data(0) {}
  octetStream(int maxlen);
  octetStream(const octetStream& os);
  octetStream& operator=(const octetStream& os)
    { if (this!=&os) { assign(os); }
      return *this;
    }
  ~octetStream() { if(data) delete[] data; }
  
  int get_ptr() const     { return ptr; }
  int get_length() const  { return len; }
  octet* get_data() const { return data; }

  octetStream hash()   const;
  // output must have length at least HASH_SIZE
  void hash(octetStream& output)   const;
  // The following produces a check sum for debugging purposes
  bigint check_sum()       const;

  void concat(const octetStream& os);

  void reset_read_head()  { ptr=0; }
  /* If we reset write head then we should reset the read head as well */
  void reset_write_head() { len=0; ptr=0; }

  // Move len back num
  void rewind_write_head(int num) { len-=num; }

  bool equals(const octetStream& a) const;

  /* Append NUM random bytes from dev/random */
  void append_random(int num);

  // Append with no padding for decoding
  void append(const octet* x,const int l); 
  // Read l octets, with no padding for decoding
  void consume(octet* x,const int l);
  // Return pointer to next l octets and advance pointer
  octet* consume(int l) { octet* res = data+ptr; ptr += l; return res; }

  /* Now store and restore different types of data (with padding for decoding) */

  void store_bytes(octet* x, const int l); //not really "bytes"...
  void get_bytes(octet* ans, int& l);      //Assumes enough space in ans

  void store(unsigned int a);
  void store(int a)   { store((unsigned int) a); }
  void get(unsigned int& a);
  void get(int& a) { get((unsigned int&) a); }

  void store(const bigint& x);
  void get(bigint& ans);

  void consume(octetStream& s,int l)
    { s.resize(l);
      consume(s.data,l);
      s.len=l;
    }

  void Send(int socket_num) const;
  void Receive(int socket_num);

  friend ostream& operator<<(ostream& s,const octetStream& o);
  friend class PRNG;
};


inline void octetStream::resize(int l)
{
  if (l<mxlen) { return; }
  l=2*l;      // Overcompensate in the resize to avoid calling this a lot
  octet* nd=new octet[l];
  if (data)
    {
      memcpy(nd,data,len*sizeof(octet));
      delete[] data;
    }
  data=nd;
  mxlen=l;
}


inline void octetStream::append(const octet* x, const int l)
{
  if (len+l>mxlen)
    resize(len+l);
  memcpy(data+len,x,l*sizeof(octet));
  len+=l;
}


inline void octetStream::consume(octet* x,const int l)
{
  memcpy(x,data+ptr,l*sizeof(octet));
  ptr+=l;
}


inline void octetStream::Send(int socket_num) const
{
  octet blen[4];
  encode_length(blen,len);
  send(socket_num,blen,4);
  send(socket_num,data,len);
}


inline void octetStream::Receive(int socket_num)
{
  octet blen[4];
  receive(socket_num,blen,4);

  int nlen=decode_length(blen);
  len=0;
  resize(nlen);
  len=nlen;

  receive(socket_num,data,len);
}

#endif


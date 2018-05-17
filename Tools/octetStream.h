// (C) 2018 University of Bristol. See License.txt

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

// 32-bit length fields for compatibility
#ifndef LENGTH_SIZE
#define LENGTH_SIZE 4
#endif

#include "Networking/data.h"
#include "Networking/sockets.h"

#include <string.h>
#include <vector>
#include <stdio.h>
#include <iostream>

#include <sodium.h>
 
using namespace std;

class bigint;

class octetStream
{
  size_t len,mxlen,ptr;  // len is the "write head", ptr is the "read head"
  octet *data;

  public:

  void resize(size_t l);
  void resize_precise(size_t l);
  void clear();

  void assign(const octetStream& os);

  octetStream() : len(0), mxlen(0), ptr(0), data(0) {}
  octetStream(size_t maxlen);
  octetStream(const octetStream& os);
  octetStream& operator=(const octetStream& os)
    { if (this!=&os) { assign(os); }
      return *this;
    }
  ~octetStream() { if(data) delete[] data; }
  
  size_t get_ptr() const     { return ptr; }
  size_t get_length() const  { return len; }
  size_t get_max_length() const { return mxlen; }
  octet* get_data() const { return data; }

  bool done() const 	  { return ptr == len; }
  bool empty() const 	  { return len == 0; }
  int left() const 		  { return len - ptr; }

  octetStream hash()   const;
  // output must have length at least HASH_SIZE
  void hash(octetStream& output)   const;
  // The following produces a check sum for debugging purposes
  bigint check_sum(int req_bytes=crypto_hash_BYTES)       const;

  void concat(const octetStream& os);

  void reset_read_head()  { ptr=0; }
  /* If we reset write head then we should reset the read head as well */
  void reset_write_head() { len=0; ptr=0; }

  // Move len back num
  void rewind_write_head(size_t num) { len-=num; }

  bool equals(const octetStream& a) const;
  bool operator==(const octetStream& a) const { return equals(a); }

  /* Append NUM random bytes from dev/random */
  void append_random(size_t num);

  // Append with no padding for decoding
  void append(const octet* x,const size_t l);
  // Read l octets, with no padding for decoding
  void consume(octet* x,const size_t l);
  // Return pointer to next l octets and advance pointer
  octet* consume(size_t l) { octet* res = data+ptr; ptr += l; return res; }

  /* Now store and restore different types of data (with padding for decoding) */

  void store_bytes(octet* x, const size_t l); //not really "bytes"...
  void get_bytes(octet* ans, size_t& l);      //Assumes enough space in ans

  void store(unsigned int a) { store_int(a, 4); }
  void store(int a);
  void get(unsigned int& a) { a = get_int(4); }
  void get(int& a);

  void store(size_t a) { store_int(a, 8); }
  void get(size_t& a) { a = get_int(8); }

  void store_int(size_t a, int n_bytes);
  size_t get_int(int n_bytes);

  void store(const bigint& x);
  void get(bigint& ans);

  // works for all statically allocated types
  template <class T>
  void serialize(const T& x) { append((octet*)&x, sizeof(x)); }
  template <class T>
  void unserialize(T& x) { consume((octet*)&x, sizeof(x)); }

  void store(const vector<int>& v);
  void get(vector<int>& v);

  void consume(octetStream& s,size_t l)
    { s.resize(l);
      consume(s.data,l);
      s.len=l;
    }

  void Send(int socket_num) const;
  void Receive(int socket_num);
  void ReceiveExpected(int socket_num, size_t expected);

  // In-place authenticated encryption using sodium; key of length crypto_generichash_BYTES
  // ciphertext = Enc(message) | MAC | counter
  //
  // This is much like 'encrypt' but uses a deterministic counter for the nonce,
  // allowing enforcement of message order.
  void encrypt_sequence(const octet* key, uint64_t counter);
  void decrypt_sequence(const octet* key, uint64_t counter);

  // In-place authenticated encryption using sodium; key of length crypto_secretbox_KEYBYTES
  // ciphertext = Enc(message) | MAC | nonce
  void encrypt(const octet* key);
  void decrypt(const octet* key);

  void exchange(int send_socket, int receive_socket);

  void input(istream& s);
  void output(ostream& s);

  friend ostream& operator<<(ostream& s,const octetStream& o);
  friend class PRNG;
};


inline void octetStream::resize(size_t l)
{
  if (l<mxlen) { return; }
  l=2*l;      // Overcompensate in the resize to avoid calling this a lot
  resize_precise(l);
}


inline void octetStream::resize_precise(size_t l)
{
  if (l == mxlen)
    return;

  octet* nd=new octet[l];
  if (data)
    {
      memcpy(nd, data, min(len, l) * sizeof(octet));
      delete[] data;
    }
  data=nd;
  mxlen=l;
}


inline void octetStream::append(const octet* x, const size_t l)
{
  if (len+l>mxlen)
    resize(len+l);
  memcpy(data+len,x,l*sizeof(octet));
  len+=l;
}


inline void octetStream::consume(octet* x,const size_t l)
{
  memcpy(x,data+ptr,l*sizeof(octet));
  ptr+=l;
}


inline void octetStream::Send(int socket_num) const
{
  send(socket_num,len,LENGTH_SIZE);
  send(socket_num,data,len);
}


inline void octetStream::Receive(int socket_num)
{
  size_t nlen=0;
  receive(socket_num,nlen,LENGTH_SIZE);
  len=0;
  resize_precise(nlen);
  len=nlen;

  receive(socket_num,data,len);
}

inline void octetStream::ReceiveExpected(int socket_num, size_t expected)
{
  size_t nlen = 0;
  receive(socket_num, nlen, LENGTH_SIZE);

  if (nlen != expected) {
      cerr << "octetStream::ReceiveExpected: got " << nlen <<
              " length, expected " << expected << endl;
      throw bad_value();
  }

  len=0;
  resize(nlen);
  len=nlen;

  receive(socket_num,data,len);
}

#endif

